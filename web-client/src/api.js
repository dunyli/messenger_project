/*
 * api.js — модуль подключения к серверу через WebSocket
 *
 * Оборачивает встроенный WebSocket в удобный API:
 *   - автоматическое переподключение
 *   - колбэки на сообщения
 *   - отправка команд в формате протокола (КОМАНДА|арг1|арг2)
 */

let ws = null;
let messageCallback = null;
let currentServerUrl = '';

/*
 * connect — устанавливает WebSocket-соединение с сервером
 * serverUrl — адрес сервера (например "192.168.1.102" или "messenger-server.local")
 * onMessage — функция, вызываемая при получении сообщения
 */
export function connect(serverUrl, onMessage) {
    messageCallback = onMessage;
    currentServerUrl = serverUrl;

    const url = `ws://${serverUrl}:8080`;

    try {
        ws = new WebSocket(url);
    } catch (error) {
        console.error('Failed to create WebSocket:', error);
        if (messageCallback) {
            messageCallback('ERROR|Failed to connect to server');
        }
        return;
    }

    ws.onopen = () => {
        console.log('Connected to server:', url);
    };

    ws.onmessage = (event) => {
        if (messageCallback) {
            messageCallback(event.data);
        }
    };

    ws.onclose = () => {
        console.log('Disconnected from server');
    };

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        if (messageCallback) {
            messageCallback('ERROR|Connection error');
        }
    };
}

/*
 * send — отправляет команду на сервер в формате протокола
 * Пример: send('LOGIN', 'alice', 'mypassword')
 */
export function send(command, ...args) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const message = [command, ...args].join('|');
        ws.send(message);
    } else {
        console.error('WebSocket is not connected');
    }
}

/*
 * disconnect — закрывает соединение
 */
export function disconnect() {
    if (ws) {
        ws.close();
    }
}