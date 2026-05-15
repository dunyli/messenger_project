/*
 * logger.h — модуль логирования сервера
 * Записывает события с меткой времени в файл логов
 * Используется для отладки и выполнения требований задания (логи сервера)
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

/*
 * log_event — записывает событие в лог-файл
 * format — строка формата (как в printf)
 * ... — аргументы для подстановки в формат
 */
static inline void log_event(const char *format, ...) {
    // Получаем текущее время
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Открываем файл логов на дозапись
    FILE *log_file = fopen("/mnt/messenger/logs/server.log", "a");
    if (!log_file) return;  // если не удалось открыть — тихо выходим

    // Пишем метку времени
    fprintf(log_file, "[%s] ", timestamp);

    // Пишем само сообщение
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fclose(log_file);
}

#endif // LOGGER_H