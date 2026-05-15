/*
 * logger.c — реализация модуля логирования
 * Отвечает за запись всех событий сервера в файл логов
 * Формат записи: [YYYY-MM-DD HH:MM:SS] текст события
 */

#include <stdio.h>      // fopen, fprintf, fclose
#include <time.h>       // time, localtime, strftime
#include <stdarg.h>     // va_list, va_start, va_end
#include "logger.h"

/*
 * log_event — записывает событие в лог-файл
 * Автоматически добавляет метку времени перед сообщением
 * Если файл логов недоступен — молча завершается (не роняет сервер)
 */
void log_event(const char *format, ...) {
    // Получаем текущее время и форматируем в строку
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Открываем файл логов на дозапись
    // Путь /mnt/messenger/logs/ — общая папка, видна и из Windows
    FILE *log_file = fopen("/mnt/messenger/logs/server.log", "a");
    if (!log_file) return;  // если не удалось открыть — выходим без ошибки

    // Записываем метку времени
    fprintf(log_file, "[%s] ", timestamp);

    // Записываем переданное сообщение с аргументами
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    // Закрываем файл (гарантирует запись на диск)
    fprintf(log_file, "\n");
    fclose(log_file);
}