/*
 * client.c — реализация консольного C-клиента
 *
 * Поток-отправитель:
 *   - Читает строки из stdin
 *   - Форматирует в протокол сервера (заменяет пробелы на |)
 *   - Отправляет через send()
 *
 * Поток-получатель:
 *   - Принимает ответы от сервера через recv()
 *   - Выводит на экран с префиксом [server]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "client.h"

/*
 * sender_thread — читает команды пользователя и отправляет на сервер
 * Формат ввода: КОМАНДА аргументы (через пробел)
 * Преобразуется в: КОМАНДА|арг1|арг2|...
 */
void *sender_thread(void *arg) {
    client_data_t *data = (client_data_t *)arg;
    char buffer[BUFFER_SIZE];

    printf("=== Messenger Client ===\n");
    printf("Commands: REGISTER|LOGIN|MSG|HISTORY|LOGOUT|/quit\n");
    printf("Examples:\n");
    printf("  REGISTER alice mypassword\n");
    printf("  LOGIN alice mypassword\n");
    printf("  MSG bob Hello!\n");
    printf("  HISTORY 10\n");
    printf("  LOGOUT\n");
    printf("  /quit\n");
    printf("========================\n\n");

    while (1) {
        // Проверяем, не завершён ли клиент
        pthread_mutex_lock(&data->mutex);
        int running = data->running;
        pthread_mutex_unlock(&data->mutex);
        if (!running) break;

        // Читаем строку от пользователя
        printf("> ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        // Убираем перевод строки
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[--len] = '\0';
        }
        if (len == 0) continue;

        // Обработка локальных команд
        if (strcmp(buffer, "/quit") == 0) {
            pthread_mutex_lock(&data->mutex);
            data->running = 0;
            pthread_mutex_unlock(&data->mutex);
            break;
        }

                // Формируем сообщение для сервера
        // Формат: КОМАНДА|арг1|арг2|...
        // Первый пробел становится | (разделитель команды и аргументов)
        // Остальные пробелы внутри текста сообщения остаются
        char formatted[BUFFER_SIZE];
        strncpy(formatted, buffer, sizeof(formatted) - 1);
        formatted[sizeof(formatted) - 1] = '\0';

        // Находим первый пробел
        char *first_space = strchr(formatted, ' ');
        if (first_space != NULL) {
            *first_space = '|';  // заменяем первый пробел на |

            // Ищем второй пробел (между первым и вторым аргументом)
            char *second_space = strchr(first_space + 1, ' ');
            if (second_space != NULL) {
                *second_space = '|';  // заменяем второй пробел на |
            }
            // ВСЁ, что после второго аргумента — это текст сообщения
            // Пробелы в тексте больше не трогаем
        }

        // Добавляем перевод строки для сервера
        len = strlen(formatted);
        if (len + 2 < sizeof(formatted)) {
            formatted[len] = '\n';
            formatted[len + 1] = '\0';
        }

        // Отправляем на сервер
        ssize_t sent = send(data->socket_fd, formatted, strlen(formatted), 0);
        if (sent < 0) {
            printf("[client] Error sending message\n");
            break;
        }
    }

    return NULL;
}

/*
 * receiver_thread — принимает ответы от сервера и выводит на экран
 */
void *receiver_thread(void *arg) {
    client_data_t *data = (client_data_t *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        // Проверяем, не завершён ли клиент
        pthread_mutex_lock(&data->mutex);
        int running = data->running;
        pthread_mutex_unlock(&data->mutex);
        if (!running) break;

        // Принимаем данные от сервера
        ssize_t bytes_read = recv(data->socket_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            // Убираем перевод строки
            size_t len = strlen(buffer);
            while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
                buffer[--len] = '\0';
            }

            // Выводим ответ с красивым форматированием
            if (strncmp(buffer, "OK|", 3) == 0) {
                printf("\r[OK]    %s\n> ", buffer + 3);
            } else if (strncmp(buffer, "ERROR|", 6) == 0) {
                printf("\r[ERROR] %s\n> ", buffer + 6);
            } else {
                printf("\r[server] %s\n> ", buffer);
            }
            fflush(stdout);
        } else if (bytes_read == 0) {
            printf("\r[client] Server closed connection\n");
            pthread_mutex_lock(&data->mutex);
            data->running = 0;
            pthread_mutex_unlock(&data->mutex);
            break;
        } else {
            printf("\r[client] Error receiving data\n");
            break;
        }
    }

    return NULL;
}