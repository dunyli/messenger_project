/*
 * client_handler.c — обработка одного клиента
 *
 * На данном этапе — заглушка:
 *   - Читает сообщение от клиента (до 4096 байт)
 *   - Выводит в консоль сервера
 *   - Отправляет ответ "Server received: ..."
 *
 * В будущем здесь будет:
 *   - Парсинг протокола (LOGIN, MSG, GROUP_MSG, LOGOUT)
 *   - Аутентификация через БД
 *   - Маршрутизация сообщений другим клиентам
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "client_handler.h"
#include "config.h"
#include "logger.h"

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];

    // Читаем данные от клиента
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';  // завершаем строку

        // Убираем символ перевода строки (если есть)
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        printf("Received: %s\n", buffer);
        log_event("Received from fd=%d: %s", client_fd, buffer);

        // Отправляем ответ клиенту
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Server received: %s\n", buffer);
        send(client_fd, response, strlen(response), 0);
    } else if (bytes_read == 0) {
        // Клиент закрыл соединение
        log_event("Client fd=%d closed connection", client_fd);
    } else {
        // Ошибка чтения
        log_event("Error reading from client fd=%d", client_fd);
    }
}