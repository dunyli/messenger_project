/*
 * Login.js — экран входа/регистрации с глазком для пароля
 */
import React, { useState } from 'react';
import './Login.css';

const SERVER_IP = '192.168.1.98';

function Login({ onLogin }) {
    const [login, setLogin] = useState('');
    const [password, setPassword] = useState('');
    const [showPassword, setShowPassword] = useState(false);
    const [message, setMessage] = useState('');
    const [messageType, setMessageType] = useState('');
    const [isRegister, setIsRegister] = useState(false);
    const [loading, setLoading] = useState(false);

    const handleSubmit = () => {
        if (!login.trim()) {
            setMessage('Введите логин');
            setMessageType('error');
            return;
        }
        if (!password.trim()) {
            setMessage('Введите пароль');
            setMessageType('error');
            return;
        }
        if (password.length < 3) {
            setMessage('Пароль минимум 3 символа');
            setMessageType('error');
            return;
        }

        setLoading(true);
        setMessage('');

        const ws = new WebSocket(`ws://${SERVER_IP}:8080`);
        let handshakeDone = false;

        ws.onmessage = (event) => {
            const data = event.data;

            if (!handshakeDone && data === 'OK|Welcome to Messenger v1.0') {
                handshakeDone = true;
                const command = isRegister ? 'REGISTER' : 'LOGIN';
                ws.send([command, login, password].join('|'));
                return;
            }

            if (data.startsWith('OK|Registration') || data.startsWith('OK|Регистрация')) {
                setMessage('Регистрация успешна! Теперь войдите.');
                setMessageType('success');
                setIsRegister(false);
                setLoading(false);
                ws.close();
                return;
            }

            if (data.startsWith('OK|Welcome')) {
                ws.close();
                onLogin(login, password);
                return;
            }

            if (data.startsWith('ERROR|')) {
                setMessage(data.substring(6));
                setMessageType('error');
                setLoading(false);
                ws.close();
            }
        };

        ws.onerror = () => {
            setMessage('Ошибка подключения к серверу');
            setMessageType('error');
            setLoading(false);
            ws.close();
        };

        setTimeout(() => {
            if (ws.readyState === WebSocket.OPEN && loading) {
                setMessage('Сервер не отвечает');
                setMessageType('error');
                setLoading(false);
                ws.close();
            }
        }, 5000);
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
                        disabled={loading}
                    />
                </div>

                <div className="login-field">
                    <span className="login-field-icon">🔒</span>
                    <input
                        type={showPassword ? 'text' : 'password'}
                        placeholder="Пароль"
                        value={password}
                        onChange={(e) => setPassword(e.target.value)}
                        onKeyDown={handleKeyDown}
                        disabled={loading}
                    />
                    <span
                        className="login-eye"
                        onClick={() => setShowPassword(!showPassword)}
                    >
                        {showPassword ? '🙈' : '👁️'}
                    </span>
                </div>

                <button
                    className="login-button"
                    onClick={handleSubmit}
                    disabled={loading}
                >
                    {loading ? 'Подключение...' : isRegister ? 'Зарегистрироваться' : 'Войти'}
                </button>

                {message && (
                    <p className={`login-message ${messageType}`}>
                        {message}
                    </p>
                )}

                <p className="login-switch" onClick={() => {
                    if (loading) return;
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