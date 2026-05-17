/*
 * main.c — точка входа консольного C-клиента
 *
 * Алгоритм:
 *   1. Подключается к серверу через TCP-сокет
 *   2. Запускает поток-отправитель (читает stdin -> send)
 *   3. Запускает поток-получатель (recv -> stdout)
 *   4. Ждёт завершения потоков
 *   5. Корректно завершает соединение
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

int main(int argc, char *argv[]) {
    const char *server_ip = SERVER_IP;
    int server_port = SERVER_PORT;

    // Переопределяем адрес из аргументов командной строки
    if (argc >= 2) server_ip = argv[1];
    if (argc >= 3) server_port = atoi(argv[2]);

    // 1. Создаём сокет
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // 2. Настраиваем адрес сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address: %s\n", server_ip);
        close(sock_fd);
        return 1;
    }

    // 3. Подключаемся к серверу
    printf("Connecting to %s:%d...\n", server_ip, server_port);
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sock_fd);
        return 1;
    }
    printf("Connected!\n");

    // 4. Инициализируем данные для потоков
    client_data_t data;
    data.socket_fd = sock_fd;
    data.running = 1;
    pthread_mutex_init(&data.mutex, NULL);

    // 5. Запускаем потоки
    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, sender_thread, &data);
    pthread_create(&recv_thread, NULL, receiver_thread, &data);

    // 6. Ждём завершения потока-отправителя (пользователь ввёл /quit)
    pthread_join(send_thread, NULL);

    // 7. Сигналим потоку-получателю о завершении
    pthread_mutex_lock(&data.mutex);
    data.running = 0;
    pthread_mutex_unlock(&data.mutex);

    // 8. Закрываем сокет (это разбудит recv в потоке-получателе)
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);

    // 9. Ждём завершения потока-получателя
    pthread_join(recv_thread, NULL);

    // 10. Очистка
    pthread_mutex_destroy(&data.mutex);
    printf("Goodbye!\n");

    return 0;
}