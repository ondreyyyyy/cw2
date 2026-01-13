-- Начальные данные для системы бронирования билетов на концерты

-- Вставка пользователя admin (верифицированный профиль)
-- Пароль: adminpassword (хеш будет установлен при первом запуске сервера функцией initAdminPassword)
-- ВАЖНО: Пароль администратора должен быть изменен на безопасный в production
INSERT INTO users (login, email, full_name, password_hash, is_admin, is_verified) 
VALUES ('admin', 'neverov.andrey11102006@gmail.com', 'Администратор', 'PLACEHOLDER_WILL_BE_SET_ON_STARTUP', TRUE, TRUE)
ON CONFLICT (login) DO NOTHING;

-- Вставка площадок (3 Новосибирск, 1 Екатеринбург, 2 Москва, 2 Санкт-Петербург)
INSERT INTO venues (name, city, address, capacity, layout_type) VALUES
-- Новосибирск (3)
('Новосибирский государственный академический театр оперы и балета', 'Новосибирск', 'Красный проспект, 36', 1774, 'theater'),
('ДКЖ Новосибирск', 'Новосибирск', 'ул. Димитрова, 1', 2500, 'concert'),
('Клуб "Подземка"', 'Новосибирск', 'ул. Ленина, 7', 900, 'club'),
-- Екатеринбург (1)
('Екатеринбург-ЭКСПО', 'Екатеринбург', 'ул. Экспо, 2', 5000, 'arena'),
-- Москва (2)
('ВТБ Арена', 'Москва', 'Ленинградский проспект, 36', 35000, 'stadium'),
('Концертный зал "Зарядье"', 'Москва', 'ул. Варварка, 6', 1600, 'philharmonic'),
-- Санкт-Петербург (2)
('Ледовый Дворец', 'Санкт-Петербург', 'проспект Победы, 1', 12500, 'arena'),
('БКЗ Октябрьский', 'Санкт-Петербург', 'Лиговский проспект, 6', 3727, 'theater');

-- Вставка категорий по умолчанию для каждой площадки
-- Новосибирский театр оперы и балета (ID 1)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(1, 'Партер', 20, 30, 5000.00),
(1, 'Бельэтаж', 5, 40, 4000.00),
(1, 'Балкон 1 ярус', 5, 50, 3000.00),
(1, 'Балкон 2 ярус', 5, 50, 2000.00);

-- ДКЖ Новосибирск (ID 2)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(2, 'VIP', 5, 20, 8000.00),
(2, 'Партер', 20, 40, 4000.00),
(2, 'Балкон', 10, 50, 2500.00);

-- Клуб Подземка (ID 3)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(3, 'Танцпол', NULL, NULL, 2000.00),
(3, 'VIP', 2, 10, 5000.00),
(3, 'Балкон', 5, 20, 3000.00);

-- Екатеринбург-ЭКСПО (ID 4)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(4, 'VIP', 5, 30, 10000.00),
(4, 'Фан-зона', NULL, NULL, 5000.00),
(4, 'Сектор A', 20, 50, 3500.00),
(4, 'Сектор B', 20, 50, 3000.00);

-- ВТБ Арена (ID 5)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(5, 'Golden Circle', NULL, NULL, 20000.00),
(5, 'Фан-зона', NULL, NULL, 8000.00),
(5, 'Трибуна A', 30, 60, 5000.00),
(5, 'Трибуна B', 30, 60, 4000.00),
(5, 'Трибуна C', 30, 60, 3000.00);

-- Зарядье (ID 6)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(6, 'Партер', 15, 35, 8000.00),
(6, 'Амфитеатр', 10, 40, 6000.00),
(6, 'Балкон', 8, 45, 4000.00);

-- Ледовый Дворец (ID 7)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(7, 'VIP Lounge', 5, 25, 15000.00),
(7, 'Партер', NULL, NULL, 6000.00),
(7, 'Сектор 1', 25, 50, 4000.00),
(7, 'Сектор 2', 25, 50, 3500.00),
(7, 'Сектор 3', 25, 50, 3000.00);

-- БКЗ Октябрьский (ID 8)
INSERT INTO venue_default_categories (venue_id, category_name, rows_count, seats_per_row, default_price) VALUES
(8, 'Партер', 25, 35, 6000.00),
(8, 'Бельэтаж', 10, 40, 5000.00),
(8, 'Балкон 1', 10, 45, 4000.00),
(8, 'Балкон 2', 10, 50, 3000.00);
