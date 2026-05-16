/*
 * main.c — точка входа серверной части мессенджера
 *
 * На данном этапе:
 *   - Создаёт TCP-сокет на порту 8080
 *   - Запускает пул из 10 рабочих потоков
 *   - Принимает входящие подключения и передаёт их в пул
 *   - Потоки обрабатывают клиентов параллельно
 *
 * Дальше будет добавлено:
 *   - Протокол обмена сообщениями
 *   - Интеграция с PostgreSQL
 *   - OpenSSL-шифрование
 */

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit, EXIT_FAILURE
#include <string.h>     // memset
#include <unistd.h>     // close
#include <signal.h>     // signal, SIGINT (для корректного завершения)
#include <sys/socket.h> // socket, bind, listen, accept, setsockopt
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons
#include "config.h"     // настройки сервера
#include "logger.h"     // логирование
#include "thread_pool.h" // пул потоков
#include "db.h"         // работа с базой данных

// Глобальная переменная для корректного завершения по Ctrl+C
static thread_pool_t *global_pool = NULL;
static int server_fd_global = 0;

/*
 * signal_handler — обрабатывает Ctrl+C (SIGINT)
 * Корректно завершает пул потоков и закрывает сокет
 */
void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down server...\n");
    log_event("Server shutdown initiated (SIGINT)");

    if (global_pool) {
        thread_pool_shutdown(global_pool);
    }
    if (server_fd_global > 0) {
        close(server_fd_global);
    }

    log_event("Server stopped");
    printf("Server stopped.\n");
    exit(0);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    thread_pool_t pool;

    // Регистрируем обработчик Ctrl+C
    signal(SIGINT, signal_handler);

    // === Шаг 1: Создаём TCP-сокет ===
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // === Шаг 2: Настраиваем адрес сервера ===
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // === Шаг 3: Привязываем сокет ===
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // === Шаг 4: Слушаем порт ===
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Сохраняем для signal_handler
    server_fd_global = server_fd;

    // === Шаг 5: Запускаем пул потоков ===
    thread_pool_init(&pool);
    global_pool = &pool;

    // === Подключаемся к базе данных ===
    PGconn *db_conn = db_connect();
    if (db_conn) {
        db_log_server_event(db_conn, "server_start",
                            "Server started on port 8080");
        // Не закрываем соединение — оно понадобится для обработки клиентов
    }

    printf("Server started on port %d\n", SERVER_PORT);
    log_event("Server started on port %d", SERVER_PORT);

    // === Шаг 6: Главный цикл — принимаем подключения ===
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd >= 0) {
            printf("New client connected (fd=%d)\n", client_fd);
            log_event("New client connected (fd=%d)", client_fd);

            // Передаём клиента в пул потоков (вместо мгновенного close)
            thread_pool_add_task(&pool, client_fd);
        }
    }

    // Сюда код не дойдёт — сервер работает бесконечно
    close(server_fd);
    return 0;
}