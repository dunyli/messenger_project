-- Таблица пользователей
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    login VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Таблица чатов (type: 'private' или 'group')
CREATE TABLE chats (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    type VARCHAR(10) NOT NULL CHECK (type IN ('private', 'group')),
    created_at TIMESTAMP DEFAULT NOW()
);

-- Участники чатов
CREATE TABLE chat_members (
    chat_id INTEGER REFERENCES chats(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    joined_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (chat_id, user_id)
);

-- Сообщения
CREATE TABLE messages (
    id SERIAL PRIMARY KEY,
    sender_id INTEGER REFERENCES users(id),
    chat_id INTEGER REFERENCES chats(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    sent_at TIMESTAMP DEFAULT NOW()
);

-- Ответы на сообщения
CREATE TABLE message_replies (
    id SERIAL PRIMARY KEY,
    message_id INTEGER REFERENCES messages(id) ON DELETE CASCADE,
    reply_to_id INTEGER REFERENCES messages(id) ON DELETE CASCADE,
    UNIQUE (message_id)
);

-- Сессии (логи входов/выходов)
CREATE TABLE sessions (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    action VARCHAR(10) NOT NULL CHECK (action IN ('login', 'logout')),
    ip_address VARCHAR(45),
    created_at TIMESTAMP DEFAULT NOW()
);

-- Логи сервера
CREATE TABLE server_logs (
    id SERIAL PRIMARY KEY,
    event_type VARCHAR(20) NOT NULL,  -- 'server_start', 'server_stop', 'error', 'info'
    description TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Индексы для быстрого поиска
CREATE INDEX idx_messages_chat_id ON messages(chat_id);
CREATE INDEX idx_messages_sender_id ON messages(sender_id);
CREATE INDEX idx_messages_sent_at ON messages(sent_at);
CREATE INDEX idx_sessions_user_id ON sessions(user_id);