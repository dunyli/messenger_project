/*
 * client_handler.h — заголовочный файл обработчика клиента
 */

#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

/*
 * handle_client — обрабатывает одного подключившегося клиента
 * Пока заглушка: читает одно сообщение, отправляет ответ, завершает
 * В будущем: протокол, аутентификация, маршрутизация сообщений
 */
void handle_client(int client_fd);

#endif // CLIENT_HANDLER_H