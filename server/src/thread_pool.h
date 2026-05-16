/*
 * thread_pool.h — заголовочный файл пула потоков
 *
 * Реализует пул из THREAD_POOL_SIZE рабочих потоков.
 * Каждый поток ожидает задачи (клиентский сокет) из общей очереди.
 * Если потоков меньше, чем клиентов — клиенты ждут в очереди.
 * пул процессов, условно 10 штук, переиспользуются;
 * если пользователей больше 10 — придётся подождать
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "config.h"

// Структура одной задачи в очереди
typedef struct {
    int client_fd;              // файловый дескриптор клиентского сокета
    struct task *next;          // указатель на следующую задачу (связный список)
} task_t;

// Структура пула потоков
typedef struct {
    pthread_t threads[THREAD_POOL_SIZE];    // массив идентификаторов потоков
    pthread_mutex_t mutex;                  // мьютекс для защиты очереди
    pthread_cond_t cond;                    // условная переменная для ожидания задач
    task_t *queue_head;                     // голова очереди задач
    task_t *queue_tail;                     // хвост очереди (для быстрой вставки)
    int running;                            // флаг: пул работает (1) или завершается (0)
} thread_pool_t;

// Инициализация пула: создаёт мьютекс, cond, запускает рабочие потоки
void thread_pool_init(thread_pool_t *pool);

// Добавление клиента в очередь задач
void thread_pool_add_task(thread_pool_t *pool, int client_fd);

// Остановка пула: потоки дорабатывают текущие задачи и завершаются
void thread_pool_shutdown(thread_pool_t *pool);

#endif // THREAD_POOL_H