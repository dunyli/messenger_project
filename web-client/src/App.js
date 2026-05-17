import React, { useState } from 'react';
import Login from './Login';
import Chat from './Chat';

function App() {
    const [loggedIn, setLoggedIn] = useState(false);
    const [login, setLogin] = useState('');
    const [password, setPassword] = useState('');

    const handleLogin = (userLogin, userPassword) => {
        setLogin(userLogin);
        setPassword(userPassword);
        setLoggedIn(true);
    };

    const handleLogout = () => {
        setLoggedIn(false);
        setLogin('');
        setPassword('');
    };

    if (!loggedIn) {
        return <Login onLogin={handleLogin} />;
    }

    return <Chat login={login} password={password} onLogout={handleLogout} />;
}

export default App;