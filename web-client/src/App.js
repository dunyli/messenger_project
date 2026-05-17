/*
 * App.js — главный компонент приложения
 * Переключает экраны: Login / Chat
 */
import React, { useState, useCallback } from 'react';
import { connect } from './api';
import Login from './Login';
import Chat from './Chat';

function App() {
    const [loggedIn, setLoggedIn] = useState(false);
    const [login, setLogin] = useState('');
    const [serverUrl, setServerUrl] = useState('');

    // Обработка входящих сообщений
    const handleMessage = useCallback((data) => {
        console.log('Incoming:', data);
    }, []);

    // Подключаемся при входе
    const handleLogin = (userLogin) => {
        setLogin(userLogin);
        setLoggedIn(true);
    };

    if (!loggedIn) {
        return <Login onLogin={handleLogin} />;
    }

    return <Chat login={login} />;
}

export default App;