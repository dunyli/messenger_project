/*
 * websocket.c — реализация протокола WebSocket (RFC 6455)
 *
 * Протокол состоит из двух фаз:
 *   1. Рукопожатие (handshake) — HTTP-запрос -> HTTP 101 ответ
 *   2. Обмен данными (data framing) — кадры с заголовком + полезная нагрузка
 *
 * Структура кадра (упрощённо):
 *   Байт 0: FIN(1) + RSV(3) + OPCODE(4)
 *   Байт 1: MASK(1) + LENGTH(7)
 *   Байты 2-3 или 2-9: расширенная длина (если нужно)
 *   Байты N-N+3: ключ маски (4 байта, только от клиента)
 *   Далее: полезная нагрузка (payload)
 *
 * Демаскировка: каждый байт payload с соответствующим байтом ключа.
 *   payload[i] = masked[i] ^ mask_key[i % 4]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <openssl/sha.h>    // SHA1 для вычисления ответного ключа
#include "websocket.h"
#include "logger.h"

// строка для WebSocket-рукопожатия (RFC 6455, секция 1.3)
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/*
 * base64_encode — кодирует бинарные данные в Base64 (RFC 4648)
 * Нужна для формирования ответного ключа Accept во время рукопожатия.
 */
static int base64_encode(const unsigned char *input, size_t input_len, char *output) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i, j = 0;

    for (i = 0; i < input_len; i += 3) {
        unsigned int val = (unsigned int)input[i] << 16;
        if (i + 1 < input_len) val |= (unsigned int)input[i + 1] << 8;
        if (i + 2 < input_len) val |= (unsigned int)input[i + 2];

        output[j++] = table[(val >> 18) & 0x3F];
        output[j++] = table[(val >> 12) & 0x3F];
        output[j++] = (i + 1 < input_len) ? table[(val >> 6) & 0x3F] : '=';
        output[j++] = (i + 2 < input_len) ? table[val & 0x3F] : '=';
    }
    output[j] = '\0';
    return j;
}

/*
 * ws_handshake — выполняет рукопожатие WebSocket
 *
 * Алгоритм:
 *   1. Читаем HTTP-запрос от клиента (браузера)
 *   2. Находим заголовок Sec-WebSocket-Key
 *   3. Конкатенируем Key + Magic-строка
 *   4. Вычисляем SHA1-хеш
 *   5. Кодируем в Base64 -> Accept-ключ
 *   6. Отправляем HTTP 101 Switching Protocols с заголовком Sec-WebSocket-Accept
 */
int ws_handshake(int client_fd) {
    char request[4096];
    ssize_t bytes = recv(client_fd, request, sizeof(request) - 1, 0);
    if (bytes <= 0) {
        log_event("WebSocket handshake failed: no request");
        return 0;
    }
    request[bytes] = '\0';

    // Ищем заголовок Sec-WebSocket-Key в HTTP-запросе
    char *key_start = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_start) {
        // Это не WebSocket-запрос — возможно, обычный TCP-клиент
        log_event("Not a WebSocket request (no Sec-WebSocket-Key)");
        return 0;
    }
    key_start += 19;  // пропускаем "Sec-WebSocket-Key: "

    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) {
        log_event("WebSocket handshake failed: malformed key");
        return 0;
    }

    // Извлекаем ключ (без \r\n)
    char ws_key[256];
    size_t key_len = key_end - key_start;
    memcpy(ws_key, key_start, key_len);
    ws_key[key_len] = '\0';

    log_event("WebSocket handshake: key=%s", ws_key);

    // Формируем строку: ключ + magic
    char combined[512];
    snprintf(combined, sizeof(combined), "%s%s", ws_key, WS_MAGIC_STRING);

    // Вычисляем SHA1-хеш
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)combined, strlen(combined), sha1_hash);

    // Кодируем в Base64
    char accept_key[64];
    base64_encode(sha1_hash, SHA_DIGEST_LENGTH, accept_key);

    // Формируем HTTP-ответ 101
    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n"
             "\r\n",
             accept_key);

    send(client_fd, response, strlen(response), 0);
    log_event("WebSocket handshake successful, accept=%s", accept_key);
    return 1;
}

/*
 * ws_send — отправляет текстовый кадр клиенту
 * Формат кадра от сервера (БЕЗ маски):
 *   Байт 0: 0x81 (FIN=1, RSV1=0, RSV2=0, RSV3=0, OPCODE=0x1)
 *   Байт 1: длина payload (без маски, mask bit = 0)
 *   Байты 2-...: расширенная длина (если payload > 125)
 *   Далее: payload
 */
int ws_send(int client_fd, const char *data, size_t len) {
    unsigned char frame[10];
    size_t header_size;

    // Байт 0: FIN=1, RSV=0, OPCODE=0x1 (текстовый кадр)
    frame[0] = 0x81;

    // Байт 1 и далее: длина (без маски, MASK=0)
    if (len <= 125) {
        frame[1] = (unsigned char)len;
        header_size = 2;
    } else if (len <= 65535) {
        frame[1] = 126;                     // признак: длина в следующих 2 байтах
        frame[2] = (unsigned char)((len >> 8) & 0xFF);   // старший байт
        frame[3] = (unsigned char)(len & 0xFF);          // младший байт
        header_size = 4;
    } else {
        frame[1] = 127;                     // признак: длина в следующих 8 байтах
        for (int i = 0; i < 8; i++) {
            frame[2 + i] = (unsigned char)((len >> (56 - i * 8)) & 0xFF);
        }
        header_size = 10;
    }

    // Отправляем заголовок
    ssize_t sent = send(client_fd, frame, header_size, 0);
    if (sent < 0) {
        log_event("WebSocket send header failed");
        return -1;
    }

    // Отправляем payload (только если есть что отправлять)
    if (len > 0) {
        sent = send(client_fd, data, len, 0);
        if (sent < 0) {
            log_event("WebSocket send payload failed");
            return -1;
        }
    }

    return header_size + len;
}

/*
 * ws_recv — принимает кадр от клиента
 *
 * Формат кадра от клиента (ОБЯЗАТЕЛЬНО с маской, MASK=1):
 *   Байт 0: FIN + OPCODE
 *   Байт 1: MASK=1 + длина
 *   Байты длины (если нужно)
 *   Байты маски (4 байта)
 *   Payload (замаскированные данные)
 *
 * Демаскировка: payload[i] ^= mask[i % 4]
 */
int ws_recv(int client_fd, char *buffer, size_t buf_size) {
    unsigned char header[2];
    ssize_t bytes = recv(client_fd, header, 2, 0);
    if (bytes <= 0) return -1;

    // Байт 0: FIN + OPCODE
    unsigned char opcode = header[0] & 0x0F;

    // Проверяем тип кадра
    if (opcode == WS_OPCODE_CLOSE) {
        log_event("WebSocket close frame received");
        return 0;  // соединение закрыто
    }
    if (opcode == WS_OPCODE_PING) {
        // Отвечаем на ping (пока заглушка)
        log_event("WebSocket ping received");
        return 0;
    }

    // Байт 1: MASK + длина
    int masked = (header[1] & 0x80) != 0;
    size_t payload_len = header[1] & 0x7F;

    // Расширенная длина
    if (payload_len == 126) {
        unsigned char ext_len[2];
        recv(client_fd, ext_len, 2, 0);
        payload_len = ((size_t)ext_len[0] << 8) | ext_len[1];
    } else if (payload_len == 127) {
        unsigned char ext_len[8];
        recv(client_fd, ext_len, 8, 0);
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | ext_len[i];
        }
    }

    // Ключ маски (только если MASK=1)
    unsigned char mask_key[4] = {0, 0, 0, 0};
    if (masked) {
        recv(client_fd, mask_key, 4, 0);
    }

    // Читаем payload
    if (payload_len >= buf_size) {
        payload_len = buf_size - 1;  // ограничиваем размером буфера
    }

    unsigned char *payload = malloc(payload_len + 1);
    if (!payload) return -1;

    recv(client_fd, payload, payload_len, 0);
    payload[payload_len] = '\0';

    // Демаскировка: XOR каждого байта с соответствующим байтом маски
    if (masked) {
        for (size_t i = 0; i < payload_len; i++) {
            payload[i] ^= mask_key[i % 4];
        }
    }

    // Копируем результат в выходной буфер
    memcpy(buffer, payload, payload_len);
    buffer[payload_len] = '\0';
    free(payload);

    return payload_len;
}