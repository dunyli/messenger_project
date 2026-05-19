/*
 * protocol.h — заголовочный файл протокола обмена
 *
 * Определяет формат команд, которыми обмениваются клиент и сервер.
 * Протокол текстовый, разделитель — вертикальная черта (|).
 *
 * Формат сообщений:
 *   Клиент -> Сервер:  КОМАНДА|параметр1|параметр2|...
 *   Сервер -> Клиент:  OK|данные\n   или   ERROR|код|описание\n
 *
 * Поддерживаемые команды:
 *   REGISTER|login|password
 *   LOGIN|login|password
 *   MSG|recipient_login|text
 *   HISTORY|recipient_login|limit
 *   LOGOUT
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>     // size_t

// Разделитель полей в сообщении
#define PROTOCOL_DELIMITER "|"

// Коды команд (целочисленные для switch-case)
typedef enum {
    CMD_UNKNOWN = 0,    // неизвестная команда
    CMD_CHECK_USER,     // проверка существования пользователя
    CMD_REGISTER,       // регистрация нового пользователя
    CMD_LOGIN,          // вход в систему
    CMD_MSG,            // отправка личного сообщения
    CMD_HISTORY,        // запрос истории сообщений
    CMD_LOGOUT          // выход из системы
} command_type_t;

// Структура распарсенного сообщения
typedef struct {
    command_type_t type;                // тип команды
    char args[4][1024];                 // массив аргументов (до 4 полей)
    int args_count;                     // количество переданных аргументов
} parsed_message_t;

/*
 * protocol_parse — разбирает сырую строку от клиента
 * raw_message — входная строка (из сокета)
 * msg         — указатель на структуру для результата
 * Возвращает 1 при успешном разборе, 0 при неверном формате
 */
int protocol_parse(const char *raw_message, parsed_message_t *msg);

/*
 * protocol_build_response — формирует ответ сервера
 * status  — "OK" или "ERROR"
 * body    — тело ответа
 * buffer  — выходной буфер
 * buf_size — размер буфера
 */
void protocol_build_response(const char *status, const char *body,
                             char *buffer, size_t buf_size);

#endif // PROTOCOL_H