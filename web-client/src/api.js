/*
 * api.js — модуль подключения к серверу через WebSocket
 */
let ws = null;
let messageCallback = null;

const SERVER_IP = '192.168.1.98';

export function connect(onMessage) {
    // Если уже подключены — закрываем старый и создаём новый
    if (ws) {
        ws.onmessage = null;
        ws.onclose = null;
        ws.onerror = null;
        ws.close();
        ws = null;
    }

    messageCallback = onMessage;
    const url = `ws://${SERVER_IP}:8080`;

    try {
        ws = new WebSocket(url);
    } catch (error) {
        console.error('Failed to create WebSocket:', error);
        if (messageCallback) messageCallback('ERROR|Failed to connect');
        return;
    }

    ws.onopen = () => {
        console.log('Connected to', url);
    };

    ws.onmessage = (event) => {
        console.log('Received:', event.data);
        if (messageCallback) {
            messageCallback(event.data);
        }
    };

    ws.onclose = () => {
        console.log('Disconnected');
        ws = null;
    };

    ws.onerror = () => {
        console.error('WebSocket error');
        if (messageCallback) messageCallback('ERROR|Connection error');
    };
}

export function send(command, ...args) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const message = [command, ...args].join('|');
        console.log('Sending:', message);
        ws.send(message);
    } else {
        console.error('WebSocket not ready');
    }
}

export function disconnect() {
    if (ws) {
        ws.close();
        ws = null;
    }
}