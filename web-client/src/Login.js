/*
 * Login.js — красивый экран входа/регистрации
 */
import React, { useState } from 'react';
import { connect, send, getWs } from './api';
import './Login.css';

function Login({ onLogin }) {
    const [login, setLogin] = useState('');
    const [password, setPassword] = useState('');
    const [message, setMessage] = useState('');
    const [isRegister, setIsRegister] = useState(false);
    const [loading, setLoading] = useState(false);

        const handleResponse = (data) => {
        setLoading(false);
        if (data.startsWith('OK|Welcome')) {
            onLogin(login);
        } else if (data.startsWith('ERROR|')) {
            setMessage(data.substring(6));
        }
    };

    const handleSubmit = () => {
        if (!login.trim() || !password.trim()) {
            setMessage('Заполните все поля');
            return;
        }
        if (password.length < 3) {
            setMessage('Пароль минимум 3 символа');
            return;
        }

        setLoading(true);
        setMessage('');
        connect(handleResponse);

        const command = isRegister ? 'REGISTER' : 'LOGIN';
        // Даём WebSocket время на подключение перед отправкой
        const trySend = () => {
            const ws = getWs();
            if (ws && ws.readyState === WebSocket.OPEN) {
                send(command, login, password);
            } else {
                setTimeout(trySend, 100);
            }
        };
        setTimeout(trySend, 300);
    };

    const handleKeyDown = (e) => {
        if (e.key === 'Enter') handleSubmit();
    };

    return (
        <div className="login-page">
            <div className="login-card">
                <div className="login-icon">💬</div>
                <h1>Messenger</h1>
                <p className="login-subtitle">
                    {isRegister ? 'Создать аккаунт' : 'Войти в аккаунт'}
                </p>

                <div className="login-field">
                    <span className="login-field-icon">👤</span>
                    <input
                        type="text"
                        placeholder="Логин"
                        value={login}
                        onChange={(e) => setLogin(e.target.value)}
                        onKeyDown={handleKeyDown}
                    />
                </div>

                <div className="login-field">
                    <span className="login-field-icon">🔒</span>
                    <input
                        type="password"
                        placeholder="Пароль"
                        value={password}
                        onChange={(e) => setPassword(e.target.value)}
                        onKeyDown={handleKeyDown}
                    />
                </div>

                <button
                    className="login-button"
                    onClick={handleSubmit}
                    disabled={loading}
                >
                    {loading ? 'Подключение...' : isRegister ? 'Зарегистрироваться' : 'Войти'}
                </button>

                {message && (
                    <p className={`login-message ${message.includes('успешно') ? 'success' : 'error'}`}>
                        {message}
                    </p>
                )}

                <p className="login-switch" onClick={() => {
                    setIsRegister(!isRegister);
                    setMessage('');
                }}>
                    {isRegister ? 'Уже есть аккаунт? Войти' : 'Нет аккаунта? Зарегистрироваться'}
                </p>
            </div>
        </div>
    );
}

export default Login;