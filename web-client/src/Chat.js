/*
 * Chat.js — основной экран мессенджера
 * Слева: список чатов + кнопка "Новый чат"
 * Справа: окно сообщений выбранного чата
 */
import React, { useState, useEffect, useRef, useCallback } from 'react';
import './Chat.css';

const SERVER_IP = '192.168.1.98';

function Chat({ login, password, onLogout }) {
    const [chats, setChats] = useState([]);
    const [activeChat, setActiveChat] = useState(null);
    const [messages, setMessages] = useState({});
    const [newChatUser, setNewChatUser] = useState('');
    const [inputText, setInputText] = useState('');
    const [showNewChat, setShowNewChat] = useState(false);
    const wsRef = useRef(null);
    const messagesEndRef = useRef(null);

    // Прокрутка вниз
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages, activeChat]);

    // Подключение WebSocket
    const connectWs = useCallback(() => {
        const ws = new WebSocket(`ws://${SERVER_IP}:8080`);
        let handshakeDone = false;

        ws.onopen = () => {
            console.log('Chat connected');
        };

        ws.onmessage = (event) => {
            const data = event.data;

            // Ждём приветствия, затем логинимся
            if (!handshakeDone && data === 'OK|Welcome to Messenger v1.0') {
                handshakeDone = true;
                ws.send(`LOGIN|${login}|${password}`);
                return;
            }

            // Ответ на логин
            if (data.startsWith('OK|Welcome')) {
                console.log('Logged in chat');
                return;
            }

            // Входящее сообщение
            if (data.startsWith('OK|New message from')) {
                const body = data.substring(3);
                const match = body.match(/New message from (.+): (.+)/);
                if (match) {
                    const sender = match[1];
                    const text = match[2];
                    const chatKey = `private_${sender}`;

                    // Добавляем чат, если нет
                    setChats(prev => {
                        if (!prev.find(c => c.key === chatKey)) {
                            return [...prev, { key: chatKey, name: sender, type: 'private' }];
                        }
                        return prev;
                    });

                    // Добавляем сообщение
                    setMessages(prev => ({
                        ...prev,
                        [chatKey]: [...(prev[chatKey] || []), { sender, text, time: new Date().toLocaleTimeString() }]
                    }));
                }
                return;
            }

            // Ответ на MSG (подтверждение)
            if (data.startsWith('OK|Message sent')) {
                return;
            }

            // Ошибка
            if (data.startsWith('ERROR|')) {
                console.log('Error:', data.substring(6));
            }
        };

        ws.onclose = () => {
            console.log('Chat disconnected');
            setTimeout(connectWs, 3000);
        };

        wsRef.current = ws;
        return ws;
    }, [login]);

    // Подключаемся при запуске
    useEffect(() => {
        const ws = connectWs();
        return () => ws.close();
    }, [connectWs]);

    // Отправка сообщения
    const handleSend = () => {
        if (!inputText.trim() || !activeChat) return;

        const ws = wsRef.current;
        if (ws && ws.readyState === WebSocket.OPEN) {
            const recipient = activeChat.name;
            ws.send(`MSG|${recipient}|${inputText}`);

            // Добавляем своё сообщение локально
            const msg = { sender: 'Вы', text: inputText, time: new Date().toLocaleTimeString(), outgoing: true };
            setMessages(prev => ({
                ...prev,
                [activeChat.key]: [...(prev[activeChat.key] || []), msg]
            }));
            setInputText('');
        }
    };

    // Начать новый чат
    const startNewChat = () => {
        if (!newChatUser.trim()) return;
        const chatKey = `private_${newChatUser}`;
        if (!chats.find(c => c.key === chatKey)) {
            const newChat = { key: chatKey, name: newChatUser, type: 'private' };
            setChats(prev => [...prev, newChat]);
            setActiveChat(newChat);
        } else {
            setActiveChat(chats.find(c => c.key === chatKey));
        }
        setNewChatUser('');
        setShowNewChat(false);
    };

    const handleKeyDown = (e) => {
        if (e.key === 'Enter') handleSend();
    };

    return (
        <div className="chat-app">
            {/* Левая панель: список чатов */}
            <div className="chat-sidebar">
                <div className="sidebar-header">
                    <span>👤 {login}</span>
                    <button onClick={onLogout} className="btn-logout">Выйти</button>
                </div>

                <div className="sidebar-chats">
                    {chats.map(chat => (
                        <div
                            key={chat.key}
                            className={`chat-item ${activeChat?.key === chat.key ? 'active' : ''}`}
                            onClick={() => setActiveChat(chat)}
                        >
                            <span className="chat-avatar">👤</span>
                            <div className="chat-item-info">
                                <div className="chat-item-name">{chat.name}</div>
                                <div className="chat-item-last">
                                    {messages[chat.key]?.slice(-1)[0]?.text?.slice(0, 30) || 'Нет сообщений'}
                                </div>
                            </div>
                        </div>
                    ))}
                </div>

                <div className="sidebar-new-chat">
                    {showNewChat ? (
                        <div className="new-chat-form">
                            <input
                                type="text"
                                placeholder="Логин пользователя"
                                value={newChatUser}
                                onChange={(e) => setNewChatUser(e.target.value)}
                                onKeyDown={(e) => e.key === 'Enter' && startNewChat()}
                            />
                            <button onClick={startNewChat}>Начать</button>
                            <button onClick={() => setShowNewChat(false)}>✕</button>
                        </div>
                    ) : (
                        <button className="btn-new-chat" onClick={() => setShowNewChat(true)}>
                            + Новый чат
                        </button>
                    )}
                </div>
            </div>

            {/* Правая панель: сообщения */}
            <div className="chat-main">
                {activeChat ? (
                    <>
                        <div className="chat-main-header">
                            <span>💬 {activeChat.name}</span>
                        </div>
                        <div className="chat-messages">
                            {(messages[activeChat.key] || []).map((msg, i) => (
                                <div key={i} className={`msg ${msg.outgoing ? 'out' : 'in'}`}>
                                    <div className="msg-sender">{msg.sender}</div>
                                    <div className="msg-text">{msg.text}</div>
                                    <div className="msg-time">{msg.time}</div>
                                </div>
                            ))}
                            <div ref={messagesEndRef} />
                        </div>
                        <div className="chat-input-bar">
                            <input
                                type="text"
                                placeholder="Сообщение..."
                                value={inputText}
                                onChange={(e) => setInputText(e.target.value)}
                                onKeyDown={handleKeyDown}
                            />
                            <button onClick={handleSend}>➤</button>
                        </div>
                    </>
                ) : (
                    <div className="chat-no-chat">
                        <p>Выберите чат или начните новый</p>
                    </div>
                )}
            </div>
        </div>
    );
}

export default Chat;