/*
 * ticketservice.cpp - Реализация класса TicketService
 * Сервис работы с билетами и бронированием
 */

#include "ticketservice.h"

using namespace std;
using json = nlohmann::json;

// Получение единственного экземпляра класса
TicketService& TicketService::getInstance() {
    static TicketService instance;
    return instance;
}

// Бронирование билета
json TicketService::bookTicket(int userId, int ticketId, int eventId) {
    lock_guard<mutex> lock(bookingMutex);
    
    json response;
    
    try {
        // Проверка доступности билета
        auto ticketResult = Database::getInstance().executeQuery("get_ticket_by_id", 
            {to_string(ticketId)});
        
        if (ticketResult.empty()) {
            response["success"] = false;
            response["error"] = "Билет не найден";
            return response;
        }

        if (!ticketResult[0]["is_available"].as<bool>()) {
            response["success"] = false;
            response["error"] = "Билет уже забронирован";
            return response;
        }

        // Бронирование билета
        auto bookingResult = Database::getInstance().executeQuery("book_ticket", 
            {to_string(userId), to_string(ticketId), to_string(eventId)});
        
        if (bookingResult.empty()) {
            response["success"] = false;
            response["error"] = "Ошибка бронирования";
            return response;
        }

        int bookingId = bookingResult[0]["id"].as<int>();

        // Отметка билета как недоступного
        Database::getInstance().executeQuery("mark_ticket_unavailable", 
            {to_string(ticketId)});

        // Обновление количества доступных мест в категории
        int categoryId = ticketResult[0]["category_id"].as<int>();
        Database::getInstance().executeQuery("update_category_available_seats", 
            {to_string(categoryId)});

        response["success"] = true;
        response["bookingId"] = bookingId;
        response["message"] = "Билет успешно забронирован";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка бронирования: " + string(e.what());
    }
    
    return response;
}

// Отмена бронирования
json TicketService::cancelBooking(int userId, int bookingId) {
    lock_guard<mutex> lock(bookingMutex);
    
    json response;
    
    try {
        // Получение данных бронирования
        auto bookingResult = Database::getInstance().executeQuery("get_booking_by_id", 
            {to_string(bookingId), to_string(userId)});
        
        if (bookingResult.empty()) {
            response["success"] = false;
            response["error"] = "Бронирование не найдено";
            return response;
        }

        int ticketId = bookingResult[0]["ticket_id"].as<int>();
        
        // Получение данных билета для категории
        auto ticketResult = Database::getInstance().executeQuery("get_ticket_by_id", 
            {to_string(ticketId)});
        
        if (ticketResult.empty()) {
            response["success"] = false;
            response["error"] = "Билет не найден";
            return response;
        }

        int categoryId = ticketResult[0]["category_id"].as<int>();

        // Отмена бронирования
        Database::getInstance().executeQuery("cancel_booking", 
            {to_string(bookingId), to_string(userId)});

        // Восстановление доступности билета
        Database::getInstance().executeQuery("restore_ticket_availability", 
            {to_string(ticketId)});

        // Восстановление количества доступных мест в категории
        Database::getInstance().executeQuery("restore_category_available_seats", 
            {to_string(categoryId)});

        response["success"] = true;
        response["message"] = "Бронирование отменено";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка отмены бронирования: " + string(e.what());
    }
    
    return response;
}

// Получение бронирований пользователя
json TicketService::getUserBookings(int userId) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_user_bookings", 
            {to_string(userId)});
        
        json bookings = json::array();
        for (const auto& row : result) {
            json booking;
            booking["bookingId"] = row["booking_id"].as<int>();
            booking["bookingDate"] = row["booking_date"].as<string>();
            booking["status"] = row["status"].as<string>();
            booking["ticket"] = {
                {"id", row["ticket_id"].as<int>()},
                {"rowNumber", row["row_number"].is_null() ? 0 : row["row_number"].as<int>()},
                {"seatNumber", row["seat_number"].is_null() ? 0 : row["seat_number"].as<int>()},
                {"price", row["price"].as<double>()},
                {"categoryName", row["category_name"].as<string>()}
            };
            booking["event"] = {
                {"id", row["event_id"].as<int>()},
                {"title", row["title"].as<string>()},
                {"genre", row["genre"].as<string>()},
                {"eventDate", row["event_date"].as<string>()},
                {"ageRestriction", row["age_restriction"].as<string>()},
                {"venueName", row["venue_name"].as<string>()},
                {"venueCity", row["venue_city"].as<string>()},
                {"layoutType", row["layout_type"].is_null() ? "" : row["layout_type"].as<string>()}
            };
            bookings.push_back(booking);
        }
        
        response["success"] = true;
        response["bookings"] = bookings;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения бронирований: " + string(e.what());
    }
    
    return response;
}

// Получение отфильтрованных бронирований пользователя
json TicketService::getFilteredUserBookings(int userId, const string& genre, const string& city,
                                             const string& dateFrom, const string& dateTo,
                                             const string& venue, const string& ageRestriction) {
    json response;
    
    try {
        vector<string> params = {
            to_string(userId),
            genre.empty() ? "NULL" : genre,
            city.empty() ? "NULL" : city,
            dateFrom.empty() ? "NULL" : dateFrom,
            dateTo.empty() ? "NULL" : dateTo,
            venue.empty() ? "NULL" : venue,
            ageRestriction.empty() ? "NULL" : ageRestriction
        };
        
        auto result = Database::getInstance().executeQuery("get_user_bookings_filtered", params);
        
        json bookings = json::array();
        for (const auto& row : result) {
            json booking;
            booking["bookingId"] = row["booking_id"].as<int>();
            booking["bookingDate"] = row["booking_date"].as<string>();
            booking["status"] = row["status"].as<string>();
            booking["ticket"] = {
                {"id", row["ticket_id"].as<int>()},
                {"rowNumber", row["row_number"].is_null() ? 0 : row["row_number"].as<int>()},
                {"seatNumber", row["seat_number"].is_null() ? 0 : row["seat_number"].as<int>()},
                {"price", row["price"].as<double>()},
                {"categoryName", row["category_name"].as<string>()}
            };
            booking["event"] = {
                {"id", row["event_id"].as<int>()},
                {"title", row["title"].as<string>()},
                {"genre", row["genre"].as<string>()},
                {"eventDate", row["event_date"].as<string>()},
                {"ageRestriction", row["age_restriction"].as<string>()},
                {"venueName", row["venue_name"].as<string>()},
                {"venueCity", row["venue_city"].as<string>()},
                {"layoutType", row["layout_type"].is_null() ? "" : row["layout_type"].as<string>()}
            };
            bookings.push_back(booking);
        }
        
        response["success"] = true;
        response["bookings"] = bookings;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка фильтрации бронирований: " + string(e.what());
    }
    
    return response;
}

// Получение деталей бронирования
json TicketService::getBookingDetails(int userId, int bookingId) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_booking_by_id", 
            {to_string(bookingId), to_string(userId)});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Бронирование не найдено";
            return response;
        }

        const auto& row = result[0];
        json booking;
        booking["bookingId"] = row["booking_id"].as<int>();
        booking["bookingDate"] = row["booking_date"].as<string>();
        booking["status"] = row["status"].as<string>();
        booking["ticket"] = {
            {"id", row["ticket_id"].as<int>()},
            {"rowNumber", row["row_number"].is_null() ? 0 : row["row_number"].as<int>()},
            {"seatNumber", row["seat_number"].is_null() ? 0 : row["seat_number"].as<int>()},
            {"price", row["price"].as<double>()},
            {"categoryName", row["category_name"].as<string>()}
        };
        booking["event"] = {
            {"id", row["event_id"].as<int>()},
            {"title", row["title"].as<string>()},
            {"genre", row["genre"].as<string>()},
            {"eventDate", row["event_date"].as<string>()},
            {"ageRestriction", row["age_restriction"].as<string>()},
            {"imageUrl", row["image_url"].is_null() ? "" : row["image_url"].as<string>()},
            {"description", row["description"].is_null() ? "" : row["description"].as<string>()},
            {"venueName", row["venue_name"].as<string>()},
            {"venueCity", row["venue_city"].as<string>()},
            {"venueAddress", row["venue_address"].is_null() ? "" : row["venue_address"].as<string>()}
        };
        
        response["success"] = true;
        response["booking"] = booking;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения информации о бронировании: " + string(e.what());
    }
    
    return response;
}
