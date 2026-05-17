/*
 * client_handler.c — обработка одного клиента
 *
 * Поддерживаемые команды:
 *   REGISTER|login|password     — регистрация
 *   LOGIN|login|password        — вход
 *   MSG|recipient|text          — личное сообщение
 *   HISTORY|recipient|limit     — история переписки
 *   LOGOUT                      — выход
 *
 * Для работы с БД используется глобальное соединение db_conn,
 * определённое в main.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "client_handler.h"
#include "config.h"
#include "logger.h"
#include "protocol.h"
#include "db.h"
#include "crypto.h"
#include "online_users.h"

// Внешняя таблица онлайн-пользователей (из main.c)
extern online_users_t online_table;

// Внешнее соединение с БД (создаётся в main.c)
extern PGconn *db_conn;

// Храним информацию о залогиненном пользователе для этой сессии
static int current_user_id = -1;
static char current_user_login[64] = "";

/*
 * send_response — отправляет ответ клиенту
 */
static void send_response(int client_fd, const char *status, const char *body) {
    char response[BUFFER_SIZE];
    protocol_build_response(status, body, response, sizeof(response));
    send(client_fd, response, strlen(response), 0);
}

/*
 * handle_register — обработка команды REGISTER
 * Хеширует пароль через SHA-256 и сохраняет в БД
 */
static void handle_register(int client_fd, parsed_message_t *msg) {
    if (msg->args_count < 2) {
        send_response(client_fd, "ERROR", "Usage: REGISTER|login|password");
        return;
    }

    char *login = msg->args[0];
    char *password = msg->args[1];

    // Хешируем пароль
    char password_hash[65];
    sha256_hash(password, password_hash);

    // Пытаемся зарегистрировать
    int result = db_register_user(db_conn, login, password_hash);

    if (result == 1) {
        log_event("User registered: %s", login);
        send_response(client_fd, "OK", "Registration successful");
    } else if (result == 0) {
        send_response(client_fd, "ERROR", "Login already exists");
    } else {
        send_response(client_fd, "ERROR", "Database error");
    }
}

/*
 * handle_login — обработка команды LOGIN
 * Проверяет логин и пароль, при успехе запоминает пользователя
 */
static void handle_login(int client_fd, parsed_message_t *msg) {
    if (msg->args_count < 2) {
        send_response(client_fd, "ERROR", "Usage: LOGIN|login|password");
        return;
    }

    char *login = msg->args[0];
    char *password = msg->args[1];

    // Хешируем пароль
    char password_hash[65];
    sha256_hash(password, password_hash);

    // Проверяем аутентификацию
    int user_id = db_authenticate_user(db_conn, login, password_hash);

    if (user_id >= 0) {
        current_user_id = user_id;
        strncpy(current_user_login, login, sizeof(current_user_login) - 1);

        // Логируем вход
        db_log_session(db_conn, user_id, "login", "127.0.0.1");

        // Добавляем пользователя в таблицу онлайн
        online_users_add(&online_table, user_id, login, client_fd);

        log_event("User logged in: %s (id=%d)", login, user_id);

        char response_body[256];
        snprintf(response_body, sizeof(response_body), "Welcome, %s! Your ID: %d", login, user_id);
        send_response(client_fd, "OK", response_body);
    } else {
        send_response(client_fd, "ERROR", "Invalid login or password");
    }
}


/*
 * handle_msg — обработка команды MSG
 * Находит/создаёт чат, сохраняет сообщение в БД,
 * и пересылает получателю, если он онлайн
 */
static void handle_msg(int client_fd, parsed_message_t *msg) {
    if (current_user_id < 0) {
        send_response(client_fd, "ERROR", "Not logged in");
        return;
    }

    if (msg->args_count < 2) {
        send_response(client_fd, "ERROR", "Usage: MSG|recipient|text");
        return;
    }

    char *recipient_login = msg->args[0];
    char *text = msg->args[1];

    // 1. Находим ID получателя
    char query[512];
    char *escaped_login = PQescapeLiteral(db_conn, recipient_login, strlen(recipient_login));
    snprintf(query, sizeof(query), "SELECT id FROM users WHERE login = %s", escaped_login);
    PQfreemem(escaped_login);

    PGresult *res = PQexec(db_conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        if (PQresultStatus(res) == PGRES_TUPLES_OK) PQclear(res);
        send_response(client_fd, "ERROR", "Recipient not found");
        return;
    }
    int recipient_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    // 2. Ищем или создаём личный чат
    snprintf(query, sizeof(query),
             "SELECT c.id FROM chats c "
             "JOIN chat_members cm1 ON c.id = cm1.chat_id AND cm1.user_id = %d "
             "JOIN chat_members cm2 ON c.id = cm2.chat_id AND cm2.user_id = %d "
             "WHERE c.type = 'private'",
             current_user_id, recipient_id);

    res = PQexec(db_conn, query);
    int chat_id = -1;

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        chat_id = atoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);

    if (chat_id < 0) {
        snprintf(query, sizeof(query),
                 "INSERT INTO chats (name, type) VALUES ('private', 'private') RETURNING id");
        res = PQexec(db_conn, query);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            send_response(client_fd, "ERROR", "Failed to create chat");
            return;
        }
        chat_id = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);

        snprintf(query, sizeof(query),
                 "INSERT INTO chat_members (chat_id, user_id) VALUES (%d, %d), (%d, %d)",
                 chat_id, current_user_id, chat_id, recipient_id);
        res = PQexec(db_conn, query);
        PQclear(res);

        log_event("Created private chat id=%d for users %d and %d",
                  chat_id, current_user_id, recipient_id);
    }

    // 3. Сохраняем сообщение в БД
    int message_id = db_save_message(db_conn, current_user_id, chat_id, text);

    if (message_id < 0) {
        send_response(client_fd, "ERROR", "Failed to save message");
        return;
    }

    log_event("Message from %s to %s: %s", current_user_login, recipient_login, text);

    // 4. Отправляем подтверждение отправителю
    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "Message sent to %s (msg_id=%d)", recipient_login, message_id);
    send_response(client_fd, "OK", response_body);

    // 5. Пересылаем сообщение получателю (если онлайн)
    int recipient_fd = online_users_find_by_login(&online_table, recipient_login);
    if (recipient_fd > 0) {
        char forward[BUFFER_SIZE];
        snprintf(forward, sizeof(forward),
                 "OK|New message from %s: %s\n", current_user_login, text);
        send(recipient_fd, forward, strlen(forward), 0);
        log_event("Message forwarded to %s (fd=%d)", recipient_login, recipient_fd);
    } else {
        log_event("Recipient %s is offline, message saved for later", recipient_login);
    }
}

/*
 * handle_history — обработка команды HISTORY
 * Загружает последние N сообщений (пока только из консоли сервера)
 */
static void handle_history(int client_fd, parsed_message_t *msg) {
    if (current_user_id < 0) {
        send_response(client_fd, "ERROR", "Not logged in");
        return;
    }

    int limit = 10;  // по умолчанию 10 сообщений
    if (msg->args_count >= 1) {
        limit = atoi(msg->args[0]);
        if (limit <= 0) limit = 10;
        if (limit > 100) limit = 100;  // ограничиваем сверху
    }

    // Пока загружаем историю в консоль сервера
    // TODO: возвращать историю клиенту в виде OK|msg1,msg2,...
    db_get_chat_history(db_conn, 1, limit);

    send_response(client_fd, "OK", "History printed on server console");
}

/*
 * handle_logout — обработка команды LOGOUT
 * Сбрасывает сессию пользователя
 */
static void handle_logout(int client_fd) {
    if (current_user_id > 0) {
        db_log_session(db_conn, current_user_id, "logout", "127.0.0.1");
        log_event("User logged out: %s (id=%d)", current_user_login, current_user_id);

        current_user_id = -1;
        memset(current_user_login, 0, sizeof(current_user_login));
    }

    send_response(client_fd, "OK", "Goodbye!");
}

/*
 * handle_client — главный обработчик одного клиента
 *
 * Определяет тип подключения:
 *   - Если запрос начинается с "GET /" — это WebSocket (браузер)
 *   - Иначе — обычный TCP-клиент (C-клиент или telnet)
 *
 * WebSocket: использует ws_recv/ws_send для обмена
 * TCP: использует recv/send напрямую
 */
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int is_websocket = 0;

    // Пробуем прочитать первые данные от клиента
    ssize_t peek_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (peek_bytes <= 0) {
        close(client_fd);
        return;
    }
    buffer[peek_bytes] = '\0';

    // Определяем тип клиента
    if (strncmp(buffer, "GET /", 5) == 0) {
        // Это WebSocket-запрос от браузера
        log_event("WebSocket client detected on fd=%d", client_fd);
        if (!ws_handshake(client_fd)) {
            log_event("WebSocket handshake failed on fd=%d", client_fd);
            close(client_fd);
            return;
        }
        is_websocket = 1;

        // Отправляем приветствие через WebSocket
        const char *welcome = "OK|Welcome to Messenger v1.0";
        ws_send(client_fd, welcome, strlen(welcome));
    } else {
        // Обычный TCP-клиент
        log_event("TCP client detected on fd=%d", client_fd);
        send_response(client_fd, "OK", "Welcome to Messenger v1.0");
    }

    // Главный цикл обработки команд
    while (1) {
        ssize_t bytes_read;

        if (is_websocket) {
            bytes_read = ws_recv(client_fd, buffer, sizeof(buffer) - 1);
        } else {
            bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        }

        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';

        // Убираем перевод строки (для TCP-клиентов)
        if (!is_websocket) {
            size_t len = strlen(buffer);
            while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
                buffer[--len] = '\0';
            }
            if (len == 0) continue;
        }

        log_event("Received from fd=%d: %s", client_fd, buffer);

        // Парсим сообщение
        parsed_message_t parsed;
        if (!protocol_parse(buffer, &parsed)) {
            if (is_websocket) {
                ws_send(client_fd, "ERROR|Unknown command", 21);
            } else {
                send_response(client_fd, "ERROR", "Unknown command or bad format");
            }
            continue;
        }

        // Временно переопределяем send_response для WebSocket
        #define SEND_OR_WS(status, body) \
            do { \
                if (is_websocket) { \
                    char tmp[BUFFER_SIZE]; \
                    protocol_build_response(status, body, tmp, sizeof(tmp)); \
                    ws_send(client_fd, tmp, strlen(tmp)); \
                } else { \
                    send_response(client_fd, status, body); \
                } \
            } while(0)

        // Выполняем команду
        switch (parsed.type) {
            case CMD_REGISTER:
                handle_register(client_fd, &parsed);
                break;
            case CMD_LOGIN:
                handle_login(client_fd, &parsed);
                break;
            case CMD_MSG:
                handle_msg(client_fd, &parsed);
                break;
            case CMD_HISTORY:
                handle_history(client_fd, &parsed);
                break;
            case CMD_LOGOUT:
                handle_logout(client_fd);
                break;
            default:
                SEND_OR_WS("ERROR", "Unknown command");
                break;
        }
    }

    // Удаляем пользователя из таблицы онлайн
    online_users_remove(&online_table, client_fd);

    log_event("Client fd=%d disconnected", client_fd);
}