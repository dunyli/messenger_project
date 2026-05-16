/*
 * db.c — реализация модуля работы с базой данных PostgreSQL
 *
 * Все функции используют синхронные запросы через PQexec.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "config.h"
#include "logger.h"

/*
 * db_connect — устанавливает соединение с PostgreSQL
 * Строка подключения собирается из макросов config.h
 */
PGconn *db_connect(void) {
    char conninfo[256];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s dbname=%s user=%s password=%s",
             DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASS);

    PGconn *conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        log_event("DB connection failed: %s", PQerrorMessage(conn));
        fprintf(stderr, "DB connection failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    log_event("Connected to database %s", DB_NAME);
    printf("Connected to database %s\n", DB_NAME);
    return conn;
}

/*
 * db_register_user — регистрация нового пользователя
 * Проверяет, не занят ли логин, затем вставляет запись
 */
int db_register_user(PGconn *conn, const char *login, const char *password_hash) {
    // Проверяем, существует ли уже такой логин
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id FROM users WHERE login = '%s'", login);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_event("DB error (register check): %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0) {
        // Пользователь уже существует
        PQclear(res);
        return 0;
    }
    PQclear(res);

    // Добавляем нового пользователя
    snprintf(query, sizeof(query),
             "INSERT INTO users (login, password_hash) VALUES ('%s', '%s')",
             login, password_hash);

    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        log_event("DB error (register insert): %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    log_event("User registered: %s", login);
    return 1;
}

/*
 * db_authenticate_user — проверка логина и хеша пароля
 * Возвращает user_id при совпадении, -1 при неверных данных
 */
int db_authenticate_user(PGconn *conn, const char *login, const char *password_hash) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id FROM users WHERE login = '%s' AND password_hash = '%s'",
             login, password_hash);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_event("DB error (auth): %s", PQerrorMessage(conn));
        PQclear(res);
        return -2;
    }

    if (PQntuples(res) == 0) {
        // Неверный логин или пароль
        PQclear(res);
        return -1;
    }

    int user_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    log_event("User authenticated: %s (id=%d)", login, user_id);
    return user_id;
}

/*
 * db_save_message — сохраняет сообщение в БД
 * Возвращает message_id или -1 при ошибке
 */
int db_save_message(PGconn *conn, int sender_id, int chat_id, const char *content) {
    char query[BUFFER_SIZE + 256];
    snprintf(query, sizeof(query),
             "INSERT INTO messages (sender_id, chat_id, content) "
             "VALUES (%d, %d, '%s') RETURNING id",
             sender_id, chat_id, content);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_event("DB error (save message): %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int message_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    log_event("Message saved: id=%d, sender=%d, chat=%d", message_id, sender_id, chat_id);
    return message_id;
}

/*
 * db_get_chat_history — загружает последние N сообщений чата
 * Пока выводит в консоль сервера (для отладки)
 * Позже будет возвращать данные клиенту через сокет
 */
void db_get_chat_history(PGconn *conn, int chat_id, int limit) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT u.login, m.content, m.sent_at "
             "FROM messages m JOIN users u ON m.sender_id = u.id "
             "WHERE m.chat_id = %d "
             "ORDER BY m.sent_at DESC LIMIT %d",
             chat_id, limit);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_event("DB error (history): %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    int rows = PQntuples(res);
    printf("=== Chat history (chat_id=%d, %d messages) ===\n", chat_id, rows);
    for (int i = 0; i < rows; i++) {
        printf("[%s] %s: %s\n",
               PQgetvalue(res, i, 2),   // sent_at
               PQgetvalue(res, i, 0),   // login
               PQgetvalue(res, i, 1));  // content
    }

    PQclear(res);
}

/*
 * db_log_session — записывает вход/выход пользователя
 */
void db_log_session(PGconn *conn, int user_id, const char *action, const char *ip_address) {
    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO sessions (user_id, action, ip_address) "
             "VALUES (%d, '%s', '%s')",
             user_id, action, ip_address);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        log_event("DB error (log session): %s", PQerrorMessage(conn));
    }
    PQclear(res);
}

/*
 * db_log_server_event — записывает событие сервера (запуск, остановка, ошибка)
 */
void db_log_server_event(PGconn *conn, const char *event_type, const char *description) {
    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO server_logs (event_type, description) "
             "VALUES ('%s', '%s')",
             event_type, description);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        log_event("DB error (log server event): %s", PQerrorMessage(conn));
    }
    PQclear(res);
}

/*
 * db_disconnect — корректное закрытие соединения с БД
 */
void db_disconnect(PGconn *conn) {
    if (conn) {
        PQfinish(conn);
        log_event("Disconnected from database");
        printf("Disconnected from database\n");
    }
}