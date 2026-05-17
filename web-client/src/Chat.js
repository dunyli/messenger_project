/*
 * Chat.js — основной экран чата
 *
 * Функции:
 *   - выбор собеседника
 *   - отправка сообщений
 *   - отображение входящих сообщений в реальном времени
 */
import React, { useState, useEffect, useRef } from 'react';
import { send } from './api';
import './Chat.css';

function Chat({ login }) {
    const [recipient, setRecipient] = useState('');
    const [text, setText] = useState('');
    const [messages, setMessages] = useState([]);
    const messagesEndRef = useRef(null);

    // Автопрокрутка вниз
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    // Добавляем сообщение в историю
    const addMessage = (sender, content, isOutgoing) => {
        setMessages(prev => [...prev, {
            sender,
            content,
            isOutgoing,
            time: new Date().toLocaleTimeString()
        }]);
    };

    // Отправка сообщения
    const handleSend = () => {
        if (!recipient || !text) return;

        send('MSG', recipient, text);
        addMessage('Вы', text, true);
        setText('');
    };

    // Обработка Enter
    const handleKeyDown = (e) => {
        if (e.key === 'Enter') handleSend();
    };

    return (
        <div className="chat-container">
            <div className="chat-header">
                <span>👤 {login}</span>
                <button onClick={() => window.location.reload()}>Выйти</button>
            </div>

            <div className="chat-recipient">
                <input
                    type="text"
                    placeholder="Кому (логин)"
                    value={recipient}
                    onChange={(e) => setRecipient(e.target.value)}
                />
            </div>

            <div className="chat-messages">
                {messages.map((msg, i) => (
                    <div key={i} className={`message ${msg.isOutgoing ? 'outgoing' : 'incoming'}`}>
                        <div className="message-sender">{msg.sender}</div>
                        <div className="message-text">{msg.content}</div>
                        <div className="message-time">{msg.time}</div>
                    </div>
                ))}
                <div ref={messagesEndRef} />
            </div>

            <div className="chat-input">
                <input
                    type="text"
                    placeholder="Сообщение..."
                    value={text}
                    onChange={(e) => setText(e.target.value)}
                    onKeyDown={handleKeyDown}
                />
                <button onClick={handleSend}>➤</button>
            </div>
        </div>
    );
}

export default Chat;