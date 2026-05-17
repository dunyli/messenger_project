/*
 * online_users.c — реализация модуля отслеживания онлайн-пользователей
 *
 * Все операции защищены мьютексом, так как вызываются из разных потоков.
 */

#include <string.h>
#include "online_users.h"
#include "logger.h"

/*
 * online_users_init — инициализация таблицы и мьютекса
 */
void online_users_init(online_users_t *ou) {
    memset(ou->users, 0, sizeof(ou->users));
    ou->count = 0;
    pthread_mutex_init(&ou->mutex, NULL);
    log_event("Online users table initialized");
}

/*
 * online_users_add — добавляет пользователя в первую свободную ячейку
 */
int online_users_add(online_users_t *ou, int user_id, const char *login, int socket_fd) {
    pthread_mutex_lock(&ou->mutex);

    for (int i = 0; i < MAX_ONLINE_USERS; i++) {
        if (!ou->users[i].active) {
            ou->users[i].user_id = user_id;
            strncpy(ou->users[i].login, login, sizeof(ou->users[i].login) - 1);
            ou->users[i].socket_fd = socket_fd;
            ou->users[i].active = 1;
            ou->count++;

            pthread_mutex_unlock(&ou->mutex);
            log_event("User %s (id=%d) added to online table (fd=%d, total=%d)",
                      login, user_id, socket_fd, ou->count);
            return 1;
        }
    }

    pthread_mutex_unlock(&ou->mutex);
    log_event("Online users table full, cannot add user %s", login);
    return 0;
}

/*
 * online_users_remove — удаляет пользователя по сокету
 */
void online_users_remove(online_users_t *ou, int socket_fd) {
    pthread_mutex_lock(&ou->mutex);

    for (int i = 0; i < MAX_ONLINE_USERS; i++) {
        if (ou->users[i].active && ou->users[i].socket_fd == socket_fd) {
            log_event("User %s (id=%d) removed from online table (fd=%d)",
                      ou->users[i].login, ou->users[i].user_id, socket_fd);
            memset(&ou->users[i], 0, sizeof(online_user_t));
            ou->count--;
            break;
        }
    }

    pthread_mutex_unlock(&ou->mutex);
}

/*
 * online_users_find_by_login — поиск сокета по логину
 */
int online_users_find_by_login(online_users_t *ou, const char *login) {
    pthread_mutex_lock(&ou->mutex);
    int fd = -1;

    for (int i = 0; i < MAX_ONLINE_USERS; i++) {
        if (ou->users[i].active && strcmp(ou->users[i].login, login) == 0) {
            fd = ou->users[i].socket_fd;
            break;
        }
    }

    pthread_mutex_unlock(&ou->mutex);
    return fd;
}

/*
 * online_users_find_by_id — поиск сокета по ID пользователя
 */
int online_users_find_by_id(online_users_t *ou, int user_id) {
    pthread_mutex_lock(&ou->mutex);
    int fd = -1;

    for (int i = 0; i < MAX_ONLINE_USERS; i++) {
        if (ou->users[i].active && ou->users[i].user_id == user_id) {
            fd = ou->users[i].socket_fd;
            break;
        }
    }

    pthread_mutex_unlock(&ou->mutex);
    return fd;
}