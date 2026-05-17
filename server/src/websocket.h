/*
 * websocket.h — заголовочный файл WebSocket-обработчика
 *
 * Реализует протокол WebSocket (RFC 6455) поверх TCP-сокета.
 *
 *   - Нет внешних зависимостей (только стандартный C/POSIX)
 *   - Полный контроль над каждым шагом протокола
 *
 * Поддерживаемые операции:
 *   - ws_handshake()  — HTTP-рукопожатие (переход с HTTP на WebSocket)
 *   - ws_send()       — отправка текстового кадра клиенту
 *   - ws_recv()       — приём кадра от клиента с демаскировкой
 */

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stddef.h>     // size_t

// Коды операций (opcode) протокола WebSocket
#define WS_OPCODE_TEXT  0x1     // текстовый кадр
#define WS_OPCODE_CLOSE 0x8     // закрытие соединения
#define WS_OPCODE_PING  0x9     // проверка связи
#define WS_OPCODE_PONG  0xA     // ответ на ping

/*
 * ws_handshake — выполняет WebSocket-рукопожатие
 * Читает HTTP-запрос от клиента, проверяет заголовок Upgrade,
 * вычисляет ответный ключ по RFC 6455 и отправляет HTTP 101.
 *
 * client_fd — сокет клиента
 * Возвращает 1 при успехе, 0 при ошибке
 */
int ws_handshake(int client_fd);

/*
 * ws_send — отправляет текстовое сообщение клиенту
 * Обрамляет данные в WebSocket-кадр (fin=1, opcode=0x1, без маски)
 *
 * client_fd — сокет клиента
 * data      — текст для отправки
 * len       — длина текста в байтах
 * Возвращает количество отправленных байт или -1 при ошибке
 */
int ws_send(int client_fd, const char *data, size_t len);

/*
 * ws_recv — принимает сообщение от клиента
 * Читает кадр WebSocket, демаскирует данные (клиент обязан маскировать),
 * извлекает текст.
 *
 * client_fd — сокет клиента
 * buffer    — буфер для полученного текста
 * buf_size  — размер буфера
 * Возвращает длину полученного текста, 0 при закрытии, -1 при ошибке
 */
int ws_recv(int client_fd, char *buffer, size_t buf_size);

#endif // WEBSOCKET_H