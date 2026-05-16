/*
 * db.h — заголовочный файл модуля работы с базой данных
 *
 * Предоставляет функции для:
 *   - подключения к PostgreSQL
 *   - регистрации и аутентификации пользователей
 *   - сохранения и загрузки сообщений
 *   - логирования сессий и событий сервера
 */

#ifndef DB_H
#define DB_H

#include <libpq-fe.h>   // PGconn, PGresult, PQ-функции

/*
 * db_connect — устанавливает соединение с PostgreSQL
 * Возвращает указатель на PGconn при успехе, NULL при ошибке
 */
PGconn *db_connect(void);

/*
 * db_register_user — регистрирует нового пользователя
 * login         — логин
 * password_hash — SHA-256 хеш пароля
 * Возвращает 1 при успехе, 0 если пользователь уже существует, -1 при ошибке
 */
int db_register_user(PGconn *conn, const char *login, const char *password_hash);

/*
 * db_authenticate_user — проверяет логин и пароль пользователя
 * Возвращает user_id при успехе, -1 при неверных данных, -2 при ошибке БД
 */
int db_authenticate_user(PGconn *conn, const char *login, const char *password_hash);

/*
 * db_save_message — сохраняет сообщение в таблицу messages
 * sender_id — отправитель
 * chat_id   — ID чата (личного или группового)
 * content   — текст сообщения
 * Возвращает message_id при успехе, -1 при ошибке
 */
int db_save_message(PGconn *conn, int sender_id, int chat_id, const char *content);

/*
 * db_get_chat_history — загружает историю сообщений чата
 * chat_id — ID чата
 * limit   — максимальное количество загружаемых сообщений
 * Выводит результат в консоль (позже будет возвращать клиенту)
 */
void db_get_chat_history(PGconn *conn, int chat_id, int limit);

/*
 * db_log_session — записывает событие входа/выхода в таблицу sessions
 */
void db_log_session(PGconn *conn, int user_id, const char *action, const char *ip_address);

/*
 * db_log_server_event — записывает событие сервера в таблицу server_logs
 */
void db_log_server_event(PGconn *conn, const char *event_type, const char *description);

/*
 * db_disconnect — закрывает соединение с БД
 */
void db_disconnect(PGconn *conn);

#endif // DB_H