/*
 * api.js — модуль подключения к серверу через WebSocket
 */
let ws = null;
let messageCallback = null;
let isConnecting = false;

const SERVER_IP = '192.168.1.98';

export function connect(onMessage) {
    messageCallback = onMessage;

    // Если уже подключены — не пересоздаём
    if (ws && ws.readyState === WebSocket.OPEN) {
        console.log('Already connected');
        return;
    }

    // Если в процессе подключения — ждём
    if (isConnecting) {
        console.log('Connection in progress...');
        return;
    }

    isConnecting = true;
    const url = `ws://${SERVER_IP}:8080`;

    try {
        ws = new WebSocket(url);
    } catch (error) {
        console.error('Failed to create WebSocket:', error);
        isConnecting = false;
        if (messageCallback) messageCallback('ERROR|Failed to connect');
        return;
    }

    ws.onopen = () => {
        console.log('Connected to', url);
        isConnecting = false;
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

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        isConnecting = false;
        if (messageCallback) messageCallback('ERROR|Connection error');
    };
}

export function send(command, ...args) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const message = [command, ...args].join('|');
        console.log('Sending:', message);
        ws.send(message);
    } else {
        console.error('WebSocket is not connected');
    }
}

export function disconnect() {
    if (ws) {
        ws.close();
        ws = null;
    }
}

export function isConnected() {
    return ws && ws.readyState === WebSocket.OPEN;
}

export function getWs() {
    return ws;
}