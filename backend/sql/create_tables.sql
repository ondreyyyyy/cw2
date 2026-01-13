-- Создание базы данных и таблиц для системы бронирования билетов на концерты
-- Этот файл содержит одноразовые запросы для настройки

-- Создание таблицы пользователей
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    login VARCHAR(15) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    full_name VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    is_admin BOOLEAN DEFAULT FALSE,
    is_verified BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы кодов подтверждения
CREATE TABLE IF NOT EXISTS verification_codes (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) NOT NULL,
    code VARCHAR(6) NOT NULL,
    expires_at TIMESTAMP NOT NULL,
    is_used BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы площадок
CREATE TABLE IF NOT EXISTS venues (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    city VARCHAR(100) NOT NULL,
    address VARCHAR(500),
    capacity INTEGER,
    layout_type VARCHAR(50) NOT NULL DEFAULT 'standard',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы категорий билетов по умолчанию для площадок
CREATE TABLE IF NOT EXISTS venue_default_categories (
    id SERIAL PRIMARY KEY,
    venue_id INTEGER REFERENCES venues(id) ON DELETE CASCADE,
    category_name VARCHAR(100) NOT NULL,
    rows_count INTEGER,
    seats_per_row INTEGER,
    default_price DECIMAL(10, 2)
);

-- Создание таблицы мероприятий
CREATE TABLE IF NOT EXISTS events (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    genre VARCHAR(100) NOT NULL,
    venue_id INTEGER REFERENCES venues(id) ON DELETE CASCADE,
    event_date TIMESTAMP NOT NULL,
    age_restriction VARCHAR(5) NOT NULL DEFAULT '0+',
    description TEXT,
    created_by INTEGER REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы категорий билетов
CREATE TABLE IF NOT EXISTS ticket_categories (
    id SERIAL PRIMARY KEY,
    event_id INTEGER REFERENCES events(id) ON DELETE CASCADE,
    category_name VARCHAR(100) NOT NULL,
    price DECIMAL(10, 2) NOT NULL,
    total_seats INTEGER NOT NULL,
    available_seats INTEGER NOT NULL,
    rows_count INTEGER,
    seats_per_row INTEGER
);

-- Создание таблицы доступных билетов (отдельные места)
CREATE TABLE IF NOT EXISTS available_tickets (
    id SERIAL PRIMARY KEY,
    event_id INTEGER REFERENCES events(id) ON DELETE CASCADE,
    category_id INTEGER REFERENCES ticket_categories(id) ON DELETE CASCADE,
    row_number INTEGER,
    seat_number INTEGER,
    is_available BOOLEAN DEFAULT TRUE,
    price DECIMAL(10, 2) NOT NULL
);

-- Создание таблицы забронированных билетов
CREATE TABLE IF NOT EXISTS booked_tickets (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    ticket_id INTEGER REFERENCES available_tickets(id) ON DELETE CASCADE,
    event_id INTEGER REFERENCES events(id) ON DELETE CASCADE,
    booking_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    status VARCHAR(20) DEFAULT 'active',
    UNIQUE(ticket_id)
);

-- Создание индексов для ускорения запросов
CREATE INDEX IF NOT EXISTS idx_events_date ON events(event_date);
CREATE INDEX IF NOT EXISTS idx_events_genre ON events(genre);
CREATE INDEX IF NOT EXISTS idx_events_venue ON events(venue_id);
CREATE INDEX IF NOT EXISTS idx_booked_tickets_user ON booked_tickets(user_id);
CREATE INDEX IF NOT EXISTS idx_booked_tickets_status ON booked_tickets(status);
CREATE INDEX IF NOT EXISTS idx_available_tickets_event ON available_tickets(event_id);
CREATE INDEX IF NOT EXISTS idx_available_tickets_available ON available_tickets(is_available);
CREATE INDEX IF NOT EXISTS idx_verification_codes_email ON verification_codes(email);

-- Пользователь admin добавляется в seed_data.sql
