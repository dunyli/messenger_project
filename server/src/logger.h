/*
 * logger.h — заголовочный файл модуля логирования
 * Содержит объявление функции log_event
 * Реализация находится в logger.c
 */

#ifndef LOGGER_H
#define LOGGER_H

/*
 * log_event — записывает событие в лог-файл с меткой времени
 * format — строка формата (как в printf)
 * ...    — аргументы для подстановки в формат
 */
void log_event(const char *format, ...);

#endif // LOGGER_H