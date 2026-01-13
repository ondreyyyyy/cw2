# Backend - Система бронирования билетов на концерты

## Структура проекта

```
backend/
├── include/           # Заголовочные файлы (.h)
│   ├── auth.h         # Сервис авторизации
│   ├── config.h       # Загрузка конфигурации
│   ├── database.h     # Работа с базой данных
│   ├── eventservice.h # Сервис мероприятий
│   ├── httpserver.h   # HTTP сервер
│   ├── jwt.h          # Работа с JWT токенами
│   ├── password.h     # Хеширование паролей
│   ├── queryloader.h  # Загрузка SQL запросов
│   ├── ticketservice.h# Сервис билетов
│   └── json.hpp       # Библиотека nlohmann/json
├── src/               # Файлы реализации (.cpp)
│   ├── auth.cpp
│   ├── config.cpp
│   ├── database.cpp
│   ├── eventservice.cpp
│   ├── httpserver.cpp
│   ├── jwt.cpp
│   ├── main.cpp
│   ├── password.cpp
│   ├── queryloader.cpp
│   └── ticketservice.cpp
├── sql/               # SQL файлы
│   ├── create_tables.sql  # Создание таблиц
│   ├── queries.sql        # SQL запросы
│   └── seed_data.sql      # Тестовые данные
├── .env               # Переменные окружения
├── CMakeLists.txt     # Конфигурация сборки
└── README.md          # Документация
```

## Требования

- Компилятор с поддержкой C++17
- CMake 3.10+
- PostgreSQL
- libpqxx
- OpenSSL

## Установка (Ubuntu/Debian)

```bash
# Установка зависимостей
sudo apt-get update
sudo apt-get install -y build-essential cmake libpqxx-dev libssl-dev postgresql postgresql-contrib

# Сборка
mkdir build && cd build
cmake ..
make
```

## Настройка базы данных

1. Создание базы данных и пользователя:
```sql
CREATE USER concert_user WITH PASSWORD 'concertuser2025';
CREATE DATABASE concert_db OWNER concert_user;
GRANT ALL PRIVILEGES ON DATABASE concert_db TO concert_user;
```

2. Создание таблиц:
```bash
psql -U concert_user -d concert_db -f sql/create_tables.sql
```

3. (Опционально) Загрузка тестовых данных:
```bash
psql -U concert_user -d concert_db -f sql/seed_data.sql
```

## Конфигурация

Файл `.env` содержит переменные окружения:
- Настройки подключения к БД
- Порт сервера
- Секретный ключ JWT

## Запуск

```bash
./build/server
```

Сервер запустится на настроенном порту (по умолчанию: 8080).

## API

### Авторизация
- `POST /api/register` - Регистрация пользователя
- `POST /api/login` - Вход в систему
- `GET /api/me` - Получение данных текущего пользователя

### Мероприятия
- `GET /api/events` - Получение всех мероприятий (с фильтрацией)
- `GET /api/event?id=<id>` - Получение мероприятия по ID
- `GET /api/categories?eventId=<id>` - Получение категорий билетов
- `GET /api/tickets/available?eventId=<id>` - Получение доступных билетов
- `GET /api/genres` - Получение всех жанров
- `GET /api/cities` - Получение всех городов
- `GET /api/venues` - Получение всех площадок

### Бронирование
- `POST /api/book` - Бронирование билета
- `DELETE /api/book?bookingId=<id>` - Отмена бронирования
- `GET /api/bookings` - Получение бронирований пользователя
- `GET /api/booking?id=<id>` - Получение деталей бронирования
