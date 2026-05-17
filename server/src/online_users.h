/*
 * online_users.h — заголовочный файл модуля отслеживания онлайн-пользователей
 *
 * Хранит список активных соединений: какой пользователь на каком сокете.
 * Потокобезопасен: защищён мьютексом.
 */

#ifndef ONLINE_USERS_H
#define ONLINE_USERS_H

#include <pthread.h>

// Максимальное количество одновременно онлайн пользователей
#define MAX_ONLINE_USERS 100

// Запись об одном онлайн-пользователе
typedef struct {
    int user_id;            // ID пользователя (из таблицы users)
    char login[64];         // логин пользователя
    int socket_fd;          // файловый дескриптор сокета клиента
    int active;             // 1 — запись занята, 0 — свободна
} online_user_t;

// Таблица онлайн-пользователей
typedef struct {
    online_user_t users[MAX_ONLINE_USERS];  // массив записей
    pthread_mutex_t mutex;                  // мьютекс для потокобезопасности
    int count;                              // текущее количество онлайн
} online_users_t;

/*
 * online_users_init — инициализация таблицы
 */
void online_users_init(online_users_t *ou);

/*
 * online_users_add — добавляет пользователя в таблицу
 * Возвращает 1 при успехе, 0 если таблица заполнена
 */
int online_users_add(online_users_t *ou, int user_id, const char *login, int socket_fd);

/*
 * online_users_remove — удаляет пользователя по сокету
 */
void online_users_remove(online_users_t *ou, int socket_fd);

/*
 * online_users_find_by_login — находит сокет пользователя по логину
 * Возвращает socket_fd или -1, если пользователь не в сети
 */
int online_users_find_by_login(online_users_t *ou, const char *login);

/*
 * online_users_find_by_id — находит сокет пользователя по ID
 * Возвращает socket_fd или -1, если пользователь не в сети
 */
int online_users_find_by_id(online_users_t *ou, int user_id);

#endif // ONLINE_USERS_H