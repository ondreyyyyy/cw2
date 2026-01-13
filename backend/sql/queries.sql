-- Все SQL запросы для системы бронирования билетов на концерты
-- Запросы именуются в формате: -- @имя_запроса

-- =====================================================
-- Запросы для работы с пользователями
-- =====================================================

-- @get_user_by_login
SELECT id, login, email, full_name, password_hash, is_admin, is_verified FROM users WHERE login = $1;

-- @get_user_by_id
SELECT id, login, email, full_name, is_admin, is_verified FROM users WHERE id = $1;

-- @get_user_by_email
SELECT id, login, email, full_name, is_admin, is_verified FROM users WHERE email = $1;

-- @get_user_by_login_and_email
SELECT id, login, email, full_name, password_hash, is_admin FROM users WHERE login = $1 AND email = $2;

-- @create_user
INSERT INTO users (login, email, full_name, password_hash, is_verified) VALUES ($1, $2, $3, $4, FALSE) RETURNING id;

-- @create_verified_user
INSERT INTO users (login, email, full_name, password_hash, is_verified) VALUES ($1, $2, $3, $4, TRUE) RETURNING id;

-- @update_user_password
UPDATE users SET password_hash = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2;

-- @verify_user
UPDATE users SET is_verified = TRUE, updated_at = CURRENT_TIMESTAMP WHERE id = $1;

-- @check_login_exists
SELECT EXISTS(SELECT 1 FROM users WHERE login = $1) AS exists;

-- @check_email_exists
SELECT EXISTS(SELECT 1 FROM users WHERE email = $1) AS exists;

-- @set_admin_password
UPDATE users SET password_hash = $1, updated_at = CURRENT_TIMESTAMP WHERE login = 'admin';

-- =====================================================
-- Запросы для кодов подтверждения
-- =====================================================

-- @create_verification_code
INSERT INTO verification_codes (email, code, expires_at) VALUES ($1, $2, $3) RETURNING id;

-- @get_verification_code
SELECT id, email, code, expires_at, is_used FROM verification_codes 
WHERE email = $1 AND code = $2 AND is_used = FALSE AND expires_at > CURRENT_TIMESTAMP
ORDER BY created_at DESC LIMIT 1;

-- @mark_code_used
UPDATE verification_codes SET is_used = TRUE WHERE id = $1;

-- @delete_expired_codes
DELETE FROM verification_codes WHERE expires_at < CURRENT_TIMESTAMP OR is_used = TRUE;

-- =====================================================
-- Запросы для работы с мероприятиями
-- =====================================================

-- @get_all_events
SELECT e.id, e.title, e.genre, e.event_date, e.age_restriction, e.description,
       v.id AS venue_id, v.name AS venue_name, v.city AS venue_city, v.address AS venue_address, v.layout_type,
       (SELECT SUM(tc.total_seats) FROM ticket_categories tc WHERE tc.event_id = e.id) AS total_tickets,
       (SELECT SUM(tc.available_seats) FROM ticket_categories tc WHERE tc.event_id = e.id) AS available_tickets
FROM events e
JOIN venues v ON e.venue_id = v.id
WHERE e.event_date > CURRENT_TIMESTAMP
ORDER BY e.event_date ASC;

-- @get_event_by_id
SELECT e.id, e.title, e.genre, e.event_date, e.age_restriction, e.description,
       v.id AS venue_id, v.name AS venue_name, v.city AS venue_city, v.address AS venue_address, v.layout_type
FROM events e
JOIN venues v ON e.venue_id = v.id
WHERE e.id = $1;

-- @get_events_filtered
SELECT e.id, e.title, e.genre, e.event_date, e.age_restriction, e.description,
       v.id AS venue_id, v.name AS venue_name, v.city AS venue_city, v.address AS venue_address, v.layout_type,
       (SELECT SUM(tc.total_seats) FROM ticket_categories tc WHERE tc.event_id = e.id) AS total_tickets,
       (SELECT SUM(tc.available_seats) FROM ticket_categories tc WHERE tc.event_id = e.id) AS available_tickets
FROM events e
JOIN venues v ON e.venue_id = v.id
WHERE e.event_date > CURRENT_TIMESTAMP
  AND ($1::VARCHAR IS NULL OR e.genre = $1)
  AND ($2::VARCHAR IS NULL OR v.city = $2)
  AND ($3::TIMESTAMP IS NULL OR e.event_date >= $3)
  AND ($4::TIMESTAMP IS NULL OR e.event_date <= $4)
  AND ($5::VARCHAR IS NULL OR v.name = $5)
  AND ($6::VARCHAR IS NULL OR e.age_restriction = $6)
ORDER BY e.event_date ASC;

-- @create_event
INSERT INTO events (title, genre, venue_id, event_date, age_restriction, description, created_by) 
VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id;

-- @update_event
UPDATE events SET title = $1, genre = $2, venue_id = $3, event_date = $4, 
       age_restriction = $5, description = $6, updated_at = CURRENT_TIMESTAMP 
WHERE id = $7;

-- @delete_event
DELETE FROM events WHERE id = $1;

-- @update_ticket_category
UPDATE ticket_categories SET price = $1, total_seats = $2, available_seats = $3 
WHERE id = $4;

-- @delete_ticket_category
DELETE FROM ticket_categories WHERE id = $1;

-- @delete_available_tickets_by_category
DELETE FROM available_tickets WHERE category_id = $1 AND is_available = TRUE;

-- @get_event_categories_for_update
SELECT id, category_name, price, total_seats, available_seats, rows_count, seats_per_row
FROM ticket_categories WHERE event_id = $1;

-- =====================================================
-- Запросы для работы с площадками
-- =====================================================

-- @get_venues
SELECT id, name, city, address, capacity, layout_type FROM venues ORDER BY city, name;

-- @get_venues_by_city
SELECT id, name, city, address, capacity, layout_type FROM venues WHERE city = $1 ORDER BY name;

-- @get_venue_by_id
SELECT id, name, city, address, capacity, layout_type FROM venues WHERE id = $1;

-- @get_venue_default_categories
SELECT id, venue_id, category_name, rows_count, seats_per_row, default_price 
FROM venue_default_categories WHERE venue_id = $1 ORDER BY id;

-- @get_cities
SELECT DISTINCT city FROM venues ORDER BY city;

-- @get_genres
SELECT DISTINCT genre FROM events ORDER BY genre;

-- =====================================================
-- Запросы для работы с категориями и билетами
-- =====================================================

-- @get_ticket_categories_by_event
SELECT id, event_id, category_name, price, total_seats, available_seats, rows_count, seats_per_row
FROM ticket_categories
WHERE event_id = $1;

-- @create_ticket_category
INSERT INTO ticket_categories (event_id, category_name, price, total_seats, available_seats, rows_count, seats_per_row)
VALUES ($1, $2, $3, $4, $4, NULLIF($5, '0')::INTEGER, NULLIF($6, '0')::INTEGER) RETURNING id;

-- @get_available_tickets_by_event
SELECT at.id, at.event_id, at.category_id, at.row_number, at.seat_number, at.price,
       tc.category_name
FROM available_tickets at
JOIN ticket_categories tc ON at.category_id = tc.id
WHERE at.event_id = $1 AND at.is_available = TRUE;

-- @get_available_tickets_by_category
SELECT at.id, at.event_id, at.category_id, at.row_number, at.seat_number, at.price,
       tc.category_name
FROM available_tickets at
JOIN ticket_categories tc ON at.category_id = tc.id
WHERE at.event_id = $1 AND at.category_id = $2 AND at.is_available = TRUE;

-- @get_ticket_by_id
SELECT at.id, at.event_id, at.category_id, at.row_number, at.seat_number, at.price, at.is_available,
       tc.category_name
FROM available_tickets at
JOIN ticket_categories tc ON at.category_id = tc.id
WHERE at.id = $1;

-- @create_available_ticket
INSERT INTO available_tickets (event_id, category_id, row_number, seat_number, price)
VALUES ($1, $2, $3, $4, $5) RETURNING id;

-- @book_ticket
INSERT INTO booked_tickets (user_id, ticket_id, event_id) VALUES ($1, $2, $3) RETURNING id;

-- @mark_ticket_unavailable
UPDATE available_tickets SET is_available = FALSE WHERE id = $1;

-- @cancel_booking
UPDATE booked_tickets SET status = 'cancelled' WHERE id = $1 AND user_id = $2;

-- @restore_ticket_availability
UPDATE available_tickets SET is_available = TRUE WHERE id = $1;

-- @update_category_available_seats
UPDATE ticket_categories SET available_seats = available_seats - 1 WHERE id = $1 AND available_seats > 0;

-- @restore_category_available_seats
UPDATE ticket_categories SET available_seats = available_seats + 1 WHERE id = $1;

-- =====================================================
-- Запросы для работы с бронированиями
-- =====================================================

-- @get_user_bookings
SELECT bt.id AS booking_id, bt.booking_date, bt.status,
       at.id AS ticket_id, at.row_number, at.seat_number, at.price,
       tc.category_name,
       e.id AS event_id, e.title, e.genre, e.event_date, e.age_restriction,
       v.name AS venue_name, v.city AS venue_city, v.layout_type
FROM booked_tickets bt
JOIN available_tickets at ON bt.ticket_id = at.id
JOIN ticket_categories tc ON at.category_id = tc.id
JOIN events e ON bt.event_id = e.id
JOIN venues v ON e.venue_id = v.id
WHERE bt.user_id = $1 AND bt.status = 'active'
ORDER BY e.event_date ASC;

-- @get_user_bookings_filtered
SELECT bt.id AS booking_id, bt.booking_date, bt.status,
       at.id AS ticket_id, at.row_number, at.seat_number, at.price,
       tc.category_name,
       e.id AS event_id, e.title, e.genre, e.event_date, e.age_restriction,
       v.name AS venue_name, v.city AS venue_city
FROM booked_tickets bt
JOIN available_tickets at ON bt.ticket_id = at.id
JOIN ticket_categories tc ON at.category_id = tc.id
JOIN events e ON bt.event_id = e.id
JOIN venues v ON e.venue_id = v.id
WHERE bt.user_id = $1 AND bt.status = 'active'
  AND ($2::VARCHAR IS NULL OR e.genre = $2)
  AND ($3::VARCHAR IS NULL OR v.city = $3)
  AND ($4::TIMESTAMP IS NULL OR e.event_date >= $4)
  AND ($5::TIMESTAMP IS NULL OR e.event_date <= $5)
  AND ($6::VARCHAR IS NULL OR v.name = $6)
  AND ($7::VARCHAR IS NULL OR e.age_restriction = $7)
ORDER BY e.event_date ASC;

-- @get_booking_by_id
SELECT bt.id AS booking_id, bt.booking_date, bt.status,
       at.id AS ticket_id, at.row_number, at.seat_number, at.price,
       tc.category_name,
       e.id AS event_id, e.title, e.genre, e.event_date, e.age_restriction, e.description,
       v.name AS venue_name, v.city AS venue_city, v.address AS venue_address
FROM booked_tickets bt
JOIN available_tickets at ON bt.ticket_id = at.id
JOIN ticket_categories tc ON at.category_id = tc.id
JOIN events e ON bt.event_id = e.id
JOIN venues v ON e.venue_id = v.id
WHERE bt.id = $1 AND bt.user_id = $2;
