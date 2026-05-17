/*
 * client.h — заголовочный файл C-клиента
 *
 * Клиент работает в два потока:
 *   - поток отправителя: читает stdin и отправляет на сервер
 *   - поток получателя: читает ответы от сервера и выводит на stdout
 *
 * Поддерживаемые команды (ввод с клавиатуры):
 *   REGISTER login password
 *   LOGIN login password
 *   MSG recipient text
 *   HISTORY [limit]
 *   LOGOUT
 *   /quit — выход из клиента
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "client_config.h"

// Структура для передачи данных в потоки
typedef struct {
    int socket_fd;          // файловый дескриптор сокета
    int running;            // флаг: клиент работает (1) или завершается (0)
    pthread_mutex_t mutex;  // мьютекс для защиты running
} client_data_t;

/*
 * sender_thread — поток отправки сообщений
 * Читает ввод пользователя и отправляет на сервер
 */
void *sender_thread(void *arg);

/*
 * receiver_thread — поток приёма сообщений
 * Читает ответы от сервера и выводит на экран
 */
void *receiver_thread(void *arg);

#endif // CLIENT_H