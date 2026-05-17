/*
 * Login.js — экран входа/регистрации
 * Содержит поле для ввода IP-адреса сервера
 * По умолчанию: messenger-server.local (mDNS)
 */
import React, { useState } from 'react';
import { connect, send } from './api';
import './Login.css';

function Login({ onLogin }) {
    const [serverUrl, setServerUrl] = useState('messenger-server.local');
    const [login, setLogin] = useState('');
    const [password, setPassword] = useState('');
    const [message, setMessage] = useState('');

    // Обработка ответа от сервера
    const handleResponse = (data) => {
        if (data.startsWith('OK|Welcome')) {
            onLogin(login);
        } else if (data.startsWith('ERROR|')) {
            setMessage(data.substring(6));
        }
    };

    // Вход
    const handleLogin = () => {
        if (!serverUrl) {
            setMessage('Введите адрес сервера');
            return;
        }
        if (!login || !password) {
            setMessage('Введите логин и пароль');
            return;
        }
        connect(serverUrl, handleResponse);
        // Даём время на подключение, затем отправляем
        setTimeout(() => send('LOGIN', login, password), 500);
    };

    // Регистрация
    const handleRegister = () => {
        if (!serverUrl) {
            setMessage('Введите адрес сервера');
            return;
        }
        if (!login || !password) {
            setMessage('Введите логин и пароль');
            return;
        }
        connect(serverUrl, handleResponse);
        setTimeout(() => send('REGISTER', login, password), 500);
    };

    return (
        <div className="login-container">
            <h1>Messenger</h1>

            <input
                type="text"
                placeholder="Адрес сервера (IP или имя)"
                value={serverUrl}
                onChange={(e) => setServerUrl(e.target.value)}
            />

            <input
                type="text"
                placeholder="Логин"
                value={login}
                onChange={(e) => setLogin(e.target.value)}
            />
            <input
                type="password"
                placeholder="Пароль"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
            />
            <div className="login-buttons">
                <button onClick={handleLogin}>Войти</button>
                <button onClick={handleRegister}>Регистрация</button>
            </div>
            {message && <p className="login-message">{message}</p>}
        </div>
    );
}

export default Login;