/*
 * eventservice.cpp - Реализация класса EventService
 * Сервис работы с мероприятиями
 */

#include "eventservice.h"

using namespace std;
using json = nlohmann::json;

// Получение единственного экземпляра класса
EventService& EventService::getInstance() {
    static EventService instance;
    return instance;
}

// Получение всех мероприятий
json EventService::getAllEvents() {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_all_events");
        
        json events = json::array();
        for (const auto& row : result) {
            json event;
            event["id"] = row["id"].as<int>();
            event["title"] = row["title"].as<string>();
            event["genre"] = row["genre"].as<string>();
            event["eventDate"] = row["event_date"].as<string>();
            event["ageRestriction"] = row["age_restriction"].as<string>();
            event["description"] = row["description"].is_null() ? "" : row["description"].as<string>();
            
            // Информация о площадке
            event["venue"] = {
                {"id", row["venue_id"].as<int>()},
                {"name", row["venue_name"].as<string>()},
                {"city", row["venue_city"].as<string>()},
                {"address", row["venue_address"].is_null() ? "" : row["venue_address"].as<string>()},
                {"layoutType", row["layout_type"].as<string>()}
            };
            
            // Информация о билетах
            int totalTickets = row["total_tickets"].is_null() ? 0 : row["total_tickets"].as<int>();
            int availableTickets = row["available_tickets"].is_null() ? 0 : row["available_tickets"].as<int>();
            event["totalTickets"] = totalTickets;
            event["availableTickets"] = availableTickets;
            
            // Проверка на малое количество билетов (<5%)
            if (totalTickets > 0) {
                double percentage = (double)availableTickets / totalTickets * 100;
                event["lowTickets"] = percentage < 5;
            } else {
                event["lowTickets"] = false;
            }
            
            events.push_back(event);
        }
        
        response["success"] = true;
        response["events"] = events;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения мероприятий: " + string(e.what());
    }
    
    return response;
}

// Получение мероприятия по ID
json EventService::getEventById(int eventId) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_event_by_id", 
            {to_string(eventId)});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Мероприятие не найдено";
            return response;
        }

        const auto& row = result[0];
        json event;
        event["id"] = row["id"].as<int>();
        event["title"] = row["title"].as<string>();
        event["genre"] = row["genre"].as<string>();
        event["eventDate"] = row["event_date"].as<string>();
        event["ageRestriction"] = row["age_restriction"].as<string>();
        event["description"] = row["description"].is_null() ? "" : row["description"].as<string>();
        event["venue"] = {
            {"id", row["venue_id"].as<int>()},
            {"name", row["venue_name"].as<string>()},
            {"city", row["venue_city"].as<string>()},
            {"address", row["venue_address"].is_null() ? "" : row["venue_address"].as<string>()},
            {"layoutType", row["layout_type"].as<string>()}
        };
        
        response["success"] = true;
        response["event"] = event;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения мероприятия: " + string(e.what());
    }
    
    return response;
}

// Получение отфильтрованных мероприятий
json EventService::getFilteredEvents(const string& genre, const string& city,
                                      const string& dateFrom, const string& dateTo,
                                      const string& venue, const string& ageRestriction) {
    json response;
    
    try {
        vector<string> params = {
            genre.empty() ? "NULL" : genre,
            city.empty() ? "NULL" : city,
            dateFrom.empty() ? "NULL" : dateFrom,
            dateTo.empty() ? "NULL" : dateTo,
            venue.empty() ? "NULL" : venue,
            ageRestriction.empty() ? "NULL" : ageRestriction
        };
        
        auto result = Database::getInstance().executeQuery("get_events_filtered", params);
        
        json events = json::array();
        for (const auto& row : result) {
            json event;
            event["id"] = row["id"].as<int>();
            event["title"] = row["title"].as<string>();
            event["genre"] = row["genre"].as<string>();
            event["eventDate"] = row["event_date"].as<string>();
            event["ageRestriction"] = row["age_restriction"].as<string>();
            event["description"] = row["description"].is_null() ? "" : row["description"].as<string>();
            event["venue"] = {
                {"id", row["venue_id"].as<int>()},
                {"name", row["venue_name"].as<string>()},
                {"city", row["venue_city"].as<string>()},
                {"address", row["venue_address"].is_null() ? "" : row["venue_address"].as<string>()},
                {"layoutType", row["layout_type"].as<string>()}
            };
            
            // Информация о билетах
            int totalTickets = row["total_tickets"].is_null() ? 0 : row["total_tickets"].as<int>();
            int availableTickets = row["available_tickets"].is_null() ? 0 : row["available_tickets"].as<int>();
            event["totalTickets"] = totalTickets;
            event["availableTickets"] = availableTickets;
            
            // Проверка на малое количество билетов (<5%)
            if (totalTickets > 0) {
                double percentage = (double)availableTickets / totalTickets * 100;
                event["lowTickets"] = percentage < 5;
            } else {
                event["lowTickets"] = false;
            }
            
            events.push_back(event);
        }
        
        response["success"] = true;
        response["events"] = events;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка фильтрации мероприятий: " + string(e.what());
    }
    
    return response;
}

// Получение категорий билетов для мероприятия
json EventService::getTicketCategories(int eventId) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_ticket_categories_by_event", 
            {to_string(eventId)});
        
        json categories = json::array();
        for (const auto& row : result) {
            json category;
            category["id"] = row["id"].as<int>();
            category["eventId"] = row["event_id"].as<int>();
            category["categoryName"] = row["category_name"].as<string>();
            category["price"] = row["price"].as<double>();
            category["totalSeats"] = row["total_seats"].as<int>();
            category["availableSeats"] = row["available_seats"].as<int>();
            category["rowsCount"] = row["rows_count"].is_null() ? 0 : row["rows_count"].as<int>();
            category["seatsPerRow"] = row["seats_per_row"].is_null() ? 0 : row["seats_per_row"].as<int>();
            categories.push_back(category);
        }
        
        response["success"] = true;
        response["categories"] = categories;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения категорий билетов: " + string(e.what());
    }
    
    return response;
}

// Получение доступных билетов для мероприятия
json EventService::getAvailableTickets(int eventId, int categoryId) {
    json response;
    
    try {
        pqxx::result result;
        if (categoryId > 0) {
            result = Database::getInstance().executeQuery("get_available_tickets_by_category", 
                {to_string(eventId), to_string(categoryId)});
        } else {
            result = Database::getInstance().executeQuery("get_available_tickets_by_event", 
                {to_string(eventId)});
        }
        
        json tickets = json::array();
        for (const auto& row : result) {
            json ticket;
            ticket["id"] = row["id"].as<int>();
            ticket["eventId"] = row["event_id"].as<int>();
            ticket["categoryId"] = row["category_id"].as<int>();
            ticket["categoryName"] = row["category_name"].as<string>();
            ticket["rowNumber"] = row["row_number"].is_null() ? 0 : row["row_number"].as<int>();
            ticket["seatNumber"] = row["seat_number"].is_null() ? 0 : row["seat_number"].as<int>();
            ticket["price"] = row["price"].as<double>();
            tickets.push_back(ticket);
        }
        
        response["success"] = true;
        response["tickets"] = tickets;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения билетов: " + string(e.what());
    }
    
    return response;
}

// Получение списка жанров
json EventService::getGenres() {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_genres");
        
        json genres = json::array();
        for (const auto& row : result) {
            genres.push_back(row["genre"].as<string>());
        }
        
        response["success"] = true;
        response["genres"] = genres;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения жанров: " + string(e.what());
    }
    
    return response;
}

// Получение списка городов
json EventService::getCities() {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_cities");
        
        json cities = json::array();
        for (const auto& row : result) {
            cities.push_back(row["city"].as<string>());
        }
        
        response["success"] = true;
        response["cities"] = cities;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения городов: " + string(e.what());
    }
    
    return response;
}

// Получение списка площадок
json EventService::getVenues() {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_venues");
        
        json venues = json::array();
        for (const auto& row : result) {
            json venue;
            venue["id"] = row["id"].as<int>();
            venue["name"] = row["name"].as<string>();
            venue["city"] = row["city"].as<string>();
            venue["address"] = row["address"].is_null() ? "" : row["address"].as<string>();
            venue["capacity"] = row["capacity"].is_null() ? 0 : row["capacity"].as<int>();
            venue["layoutType"] = row["layout_type"].as<string>();
            venues.push_back(venue);
        }
        
        response["success"] = true;
        response["venues"] = venues;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения площадок: " + string(e.what());
    }
    
    return response;
}
