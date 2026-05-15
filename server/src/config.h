/*
 * config.h — конфигурация сервера мессенджера
 * Содержит все основные настройки: порт, лимиты, параметры БД
 */

#ifndef CONFIG_H
#define CONFIG_H

// Сетевые настройки
#define SERVER_PORT     8080        // порт, на котором сервер принимает подключения
#define MAX_CLIENTS     100         // максимальная очередь ожидающих подключений
#define THREAD_POOL_SIZE 10         // количество потоков в пуле для обработки клиентов
#define BUFFER_SIZE     4096        // размер буфера для приёма/отправки сообщений

// Параметры подключения к PostgreSQL
#define DB_HOST         "localhost"
#define DB_PORT         "5432"
#define DB_NAME         "messenger_db"
#define DB_USER         "msguser"
#define DB_PASS         "strongpass123"

#endif // CONFIG_H