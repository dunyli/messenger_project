/*
 * main.c — точка входа серверной части мессенджера
 * Создаёт TCP-сокет, слушает порт, принимает входящие подключения
 * На данном этапе — каркас: клиент принимается и сразу закрывается
 * Обработка клиентов и интеграция с БД будут добавлены позже
 */

#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <string.h>     // memset
#include <unistd.h>     // close
#include <pthread.h>    // pthread (пока зарезервировано)
#include <sys/socket.h> // socket, bind, listen, accept
#include <netinet/in.h> // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>  // inet_ntoa
#include "config.h"     // настройки сервера
#include "logger.h"     // логирование

int main() {
    int server_fd;                  // файловый дескриптор серверного сокета
    struct sockaddr_in address;     // структура для адреса сервера
    int opt = 1;                    // флаг для setsockopt (разрешить переиспользование порта)

    // 1. Создаём TCP-сокет (IPv4, потоковый)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Разрешаем переиспользовать порт после остановки сервера
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Заполняем адрес сервера
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;           // IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // слушаем на всех интерфейсах
    address.sin_port = htons(SERVER_PORT);  // порт в сетевом порядке байт

    // 3. Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Переводим сокет в режим прослушивания
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", SERVER_PORT);
    log_event("Server started on port %d", SERVER_PORT);

    // 5. Главный цикл — бесконечно принимаем подключения
    while (1) {
        // Принимаем входящее подключение
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd >= 0) {
            printf("New client connected\n");
            log_event("New client connected (fd=%d)", client_fd);

            // TODO: здесь будет создание потока или передача в пул для обработки клиента
            // Пока просто закрываем соединение
            close(client_fd);
        }
    }

    // 6. Завершение работы (до этой строки код не дойдёт — сервер работает бесконечно)
    close(server_fd);
    log_event("Server stopped");
    return 0;
}
