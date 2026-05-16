/*
 * thread_pool.c — реализация пула потоков
 *
 * Рабочий поток:
 *   1. Захватывает мьютекс
 *   2. Ждёт задачу через pthread_cond_wait
 *   3. Извлекает client_fd из очереди
 *   4. Освобождает мьютекс
 *   5. Вызывает client_handler() для обработки клиента
 *   6. Возвращается к шагу 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "thread_pool.h"
#include "logger.h"

// Внешняя функция обработки клиента (определена в client_handler.c)
extern void handle_client(int client_fd);

/*
 * worker — функция, выполняемая каждым рабочим потоком
 * Бесконечно ждёт задачи из очереди и обрабатывает их
 */
static void *worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        // Захватываем мьютекс для доступа к очереди
        pthread_mutex_lock(&pool->mutex);

        // Ждём, пока появится задача или пул не будет остановлен
        while (pool->queue_head == NULL && pool->running) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        // Если пул остановлен и очередь пуста — выходим
        if (!pool->running && pool->queue_head == NULL) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // Извлекаем первую задачу из очереди
        task_t *task = pool->queue_head;
        pool->queue_head = task->next;
        if (pool->queue_head == NULL) {
            pool->queue_tail = NULL;  // очередь опустела
        }

        pthread_mutex_unlock(&pool->mutex);

        // Обрабатываем клиента (вне критической секции)
        int client_fd = task->client_fd;
        free(task);

        log_event("Thread processing client fd=%d", client_fd);
        handle_client(client_fd);
        close(client_fd);
        log_event("Client fd=%d disconnected", client_fd);
    }

    return NULL;
}

/*
 * thread_pool_init — инициализация пула и запуск рабочих потоков
 */
void thread_pool_init(thread_pool_t *pool) {
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->running = 1;

    // Инициализируем мьютекс и условную переменную
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);

    // Создаём рабочие потоки
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&pool->threads[i], NULL, worker, pool);
    }

    log_event("Thread pool initialized with %d threads", THREAD_POOL_SIZE);
    printf("Thread pool: %d threads started\n", THREAD_POOL_SIZE);
}

/*
 * thread_pool_add_task — добавить клиента в очередь
 * Потокобезопасно: защищено мьютексом
 */
void thread_pool_add_task(thread_pool_t *pool, int client_fd) {
    // Создаём новую задачу
    task_t *task = malloc(sizeof(task_t));
    task->client_fd = client_fd;
    task->next = NULL;

    // Добавляем в конец очереди
    pthread_mutex_lock(&pool->mutex);

    if (pool->queue_tail == NULL) {
        pool->queue_head = task;
        pool->queue_tail = task;
    } else {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }

    // Сигналим одному ожидающему потоку, что есть работа
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
}

/*
 * thread_pool_shutdown — остановка пула
 * Дожидается завершения всех рабочих потоков
 */
void thread_pool_shutdown(thread_pool_t *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->running = 0;
    // Будим все потоки, чтобы они проверили флаг running
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    // Ждём завершения каждого потока
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // Удаляем мьютекс и условную переменную
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);

    log_event("Thread pool shut down");
    printf("Thread pool shut down\n");
}