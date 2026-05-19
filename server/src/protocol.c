/*
 * protocol.c — реализация парсера протокола обмена
 *
 * Разбирает текстовые команды от клиента формата:
 *   КОМАНДА|арг1|арг2|...
 *
 * Примеры:
 *   REGISTER|alice|mypassword
 *   LOGIN|alice|mypassword
 *   MSG|bob|Привет, как дела?
 *   HISTORY|bob|10
 *   LOGOUT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"
#include "config.h"

/*
 * parse_command — определяет тип команды по строке
 */
static command_type_t parse_command(const char *cmd_str) {
    if (strcmp(cmd_str, "REGISTER") == 0) return CMD_REGISTER;
    if (strcmp(cmd_str, "LOGIN") == 0)    return CMD_LOGIN;
    if (strcmp(cmd_str, "MSG") == 0)      return CMD_MSG;
    if (strcmp(cmd_str, "HISTORY") == 0)  return CMD_HISTORY;
    if (strcmp(cmd_str, "LOGOUT") == 0)   return CMD_LOGOUT;
    if (strcmp(cmd_str, "CHECK_USER") == 0) return CMD_CHECK_USER;
    return CMD_UNKNOWN;
}

/*
 * protocol_parse — разбирает сообщение от клиента
 *
 * Алгоритм:
 *   1. Копируем строку (strtok меняет оригинал)
 *   2. Разбиваем по разделителю "|"
 *   3. Первый токен — команда, остальные — аргументы
 *   4. Сохраняем в структуру parsed_message_t
 *
 * Возвращает 1 при успехе, 0 при ошибке
 */
int protocol_parse(const char *raw_message, parsed_message_t *msg) {
    if (raw_message == NULL || msg == NULL) {
        return 0;
    }

    // Копируем строку, т.к. strtok её модифицирует
    char buffer[BUFFER_SIZE];
    strncpy(buffer, raw_message, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Инициализируем структуру
    memset(msg, 0, sizeof(parsed_message_t));
    msg->type = CMD_UNKNOWN;

    // Разбираем первый токен — команду
    char *token = strtok(buffer, PROTOCOL_DELIMITER);
    if (token == NULL) {
        return 0;  // пустая строка
    }

    msg->type = parse_command(token);
    if (msg->type == CMD_UNKNOWN) {
        return 0;  // неизвестная команда
    }

    // Разбираем аргументы (до 4)
    int i = 0;
    while ((token = strtok(NULL, PROTOCOL_DELIMITER)) != NULL && i < 4) {
        strncpy(msg->args[i], token, sizeof(msg->args[i]) - 1);
        msg->args[i][sizeof(msg->args[i]) - 1] = '\0';
        i++;
    }
    msg->args_count = i;

    return 1;
}

/*
 * protocol_build_response — формирует ответ сервера
 * Формат: "STATUS|тело_ответа\n"
 */
void protocol_build_response(const char *status, const char *body,
                             char *buffer, size_t buf_size) {
    snprintf(buffer, buf_size, "%s|%s\n", status, body);
}