# TicketHub - Сервис бронирования билетов на концерты

Безопасное программное обеспечение для бронирования и фильтрации билетов на концертные мероприятия.

## Пошаговая инструкция по запуску

### 1. Установка зависимостей (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libpqxx-dev libssl-dev postgresql postgresql-contrib
```

### 2. Настройка базы данных PostgreSQL

```bash
# Запуск PostgreSQL
sudo service postgresql start

# Переключение на пользователя postgres
sudo -i -u postgres

# Создание базы данных и пользователя (в интерактивном режиме psql)
psql
```

В psql выполните:
```sql
CREATE USER concert_user WITH PASSWORD 'concertuser2025';
CREATE DATABASE concert_db OWNER concert_user;
GRANT ALL PRIVILEGES ON DATABASE concert_db TO concert_user;
\q
```

Выйдите из пользователя postgres:
```bash
exit
```

### 3. Создание таблиц и загрузка данных

```bash
# Из корневой папки проекта:
psql -h localhost -U concert_user -d concert_db -f backend/sql/create_tables.sql
# Введите пароль: concertuser2025

# Загрузка начальных данных (площадки, категории, admin)
psql -h localhost -U concert_user -d concert_db -f backend/sql/seed_data.sql
```

### 4. Сборка проекта

```bash
cd backend
mkdir -p build
cd build
cmake ..
make
```

### 5. Запуск сервера

**ВАЖНО**: Сервер нужно запускать из папки `backend`, чтобы он мог найти:
- Файл конфигурации `_env`
- SQL запросы `sql/queries.sql`
- Статические файлы `../frontend`

```bash
# Из папки backend (НЕ из build!)
cd /путь/к/проекту/backend
./build/tickethub

# Или если исполняемый файл называется server:
./build/server
```

Вы должны увидеть:
```
Конфигурация загружена
SQL запросы загружены
Подключение к базе данных установлено
Пароль администратора инициализирован
Запуск сервера на порту 8080...
Сервер запущен на порту 8080
```

### 6. Открытие сайта

Откройте в браузере: **http://localhost:8080**

Если вы видите главную страницу с надписью "tickethub" - всё работает!

### Возможные проблемы

| Проблема | Решение |
|----------|---------|
| "Доступ запрещен" или кракозябры | Запустите сервер из папки `backend`, а не из `build` |
| "Файл не найден" | Проверьте, что файлы frontend существуют в папке `../frontend` относительно backend |
| Ошибка подключения к БД | Проверьте, что PostgreSQL запущен и данные в `_env` верны |
| Порт занят | Измените `SERVER_PORT` в файле `_env` |

### Учетные данные администратора

- **Логин**: `admin`
- **Пароль**: `adminpassword`
- **Email**: `admin@tickethub.local`

**ВАЖНО**: Рекомендуется изменить пароль администратора после первого входа в production-среде.

## Структура проекта

```
├── backend/                  # Серверная часть (C++)
│   ├── include/             # Заголовочные файлы (.h)
│   │   ├── auth.h           # Сервис авторизации
│   │   ├── config.h         # Конфигурация
│   │   ├── database.h       # Работа с БД
│   │   ├── queryloader.h    # Загрузчик SQL запросов
│   │   ├── password.h       # Хеширование паролей
│   │   ├── jwt.h            # JWT токены
│   │   ├── eventservice.h   # Сервис мероприятий
│   │   ├── ticketservice.h  # Сервис билетов
│   │   ├── adminservice.h   # Сервис администратора
│   │   ├── emailservice.h   # Отправка email (SMTP)
│   │   └── httpserver.h     # HTTP сервер
│   ├── src/                 # Файлы реализации (.cpp)
│   ├── sql/                 # SQL файлы
│   │   ├── create_tables.sql   # Создание таблиц БД
│   │   ├── queries.sql         # Все SQL запросы
│   │   └── seed_data.sql       # Начальные данные
│   ├── _env                 # Переменные окружения
│   └── CMakeLists.txt       # Сборка проекта
├── frontend/                # Клиентская часть (HTML/CSS/JS)
│   ├── css/
│   │   └── styles.css       # Стили
│   ├── js/
│   │   ├── api.js          # API клиент
│   │   ├── auth.js         # Авторизация на клиенте
│   │   └── main.js         # Главная страница
│   ├── pages/
│   │   ├── login.html       # Страница входа
│   │   ├── register.html    # Страница регистрации
│   │   ├── forgot-password.html # Восстановление пароля
│   │   ├── change-password.html # Смена пароля
│   │   ├── booking-info.html    # Информация о бронировании
│   │   ├── tickets.html     # Страница билетов
│   │   ├── ticket-detail.html   # Детали билета
│   │   ├── events.html      # Страница мероприятий (бронирование)
│   │   └── admin.html       # Админ-панель
│   └── index.html           # Главная страница
└── README.md
```

## Требования

### Backend
- C++17 совместимый компилятор (g++ 7+, clang++ 5+)
- CMake 3.10+
- PostgreSQL 12+
- libpqxx (C++ библиотека для PostgreSQL)
- OpenSSL

### Frontend
- Современный веб-браузер с поддержкой ES6

## Конфигурация

Файл `backend/_env`:

```env
# База данных
DB_HOST=localhost
DB_PORT=5432
DB_NAME=concert_db
DB_USER=concert_user
DB_PASSWORD=concertuser2025

# Сервер
SERVER_PORT=8080

# JWT токены
JWT_SECRET=your_secret_key_here

# SMTP для отправки email (для Gmail используйте App Password)
SMTP_SERVER=smtp.gmail.com
SMTP_PORT=587
SMTP_USER=your_email@gmail.com
SMTP_PASSWORD=your_app_password
```

## API Endpoints

### Аутентификация
- `POST /api/send-verification-code` - Отправка кода подтверждения на email
- `POST /api/register` - Регистрация пользователя с кодом
- `POST /api/login` - Вход в систему
- `GET /api/me` - Получение данных текущего пользователя
- `POST /api/recover-password` - Восстановление пароля
- `POST /api/change-password` - Смена пароля

### Мероприятия
- `GET /api/events` - Список мероприятий (с фильтрацией)
- `GET /api/event?id=<id>` - Детали мероприятия
- `GET /api/categories?eventId=<id>` - Категории билетов
- `GET /api/tickets/available?eventId=<id>` - Доступные билеты
- `GET /api/genres` - Список жанров
- `GET /api/cities` - Список городов
- `GET /api/venues` - Список площадок

### Бронирование
- `POST /api/book` - Забронировать билет
- `DELETE /api/book?bookingId=<id>` - Отменить бронирование (вернуть билет)
- `GET /api/bookings` - Список бронирований пользователя
- `GET /api/booking?id=<id>` - Детали бронирования

### Администратор
- `GET /api/admin/venues?city=<city>` - Площадки по городу
- `GET /api/admin/venue-categories?venueId=<id>` - Категории площадки
- `POST /api/admin/event` - Создание мероприятия
- `GET /api/admin/event?id=<id>` - Получение мероприятия для редактирования
- `PUT /api/admin/event` - Обновление мероприятия
- `DELETE /api/admin/event?id=<id>` - Удаление мероприятия

## Безопасность

- Пароли хранятся в виде хешей (SHA-256 с солью)
- SQL запросы хранятся отдельно от кода
- Используются параметризованные запросы
- JWT токены для аутентификации
- Потокобезопасность через mutex
- Валидация всех входных данных
- CORS настройки для API

## Технологии

### Backend
- C++17
- PostgreSQL
- libpqxx
- OpenSSL (SHA-256, HMAC)
- Собственный HTTP сервер

### Frontend
- HTML5
- CSS3
- JavaScript (ES6+)
- Шрифты: Vollkorn, Inter (Google Fonts)

## Скриншоты

### Главная страница
<img src="https://github.com/user-attachments/assets/fc4b1acc-76c3-4eac-8ba2-91142e2b1e5a" alt="Главная страница" width="800">

### Страница входа
<img src="https://github.com/user-attachments/assets/4b2d6b73-25c8-424d-802d-54b6487baf14" alt="Страница входа" width="800">

### Страница регистрации с валидацией
<img src="https://github.com/user-attachments/assets/ae60a644-ac62-4407-9b1d-68fdc0065d93" alt="Регистрация" width="800">

### Информация о бронировании
<img src="https://github.com/user-attachments/assets/f040c8a8-2c59-4a19-8415-733137399c93" alt="Информация о бронировании" width="800">

### Главная страница (старый дизайн)
<img src="https://github.com/user-attachments/assets/babaf6ed-7f67-450a-b0e2-03f44fcc68f6" alt="Главная страница - старый дизайн" width="800">

### Информация о бронировании (старый дизайн)
<img src="https://github.com/user-attachments/assets/66f98dca-2483-4ba9-9652-1bd972d8d858" alt="Информация о бронировании - старый дизайн" width="800">