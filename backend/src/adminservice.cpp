/*
 * adminservice.cpp - Реализация класса AdminService
 * Сервис управления мероприятиями для администратора
 */

#include "adminservice.h"

using namespace std;
using json = nlohmann::json;

// Получение единственного экземпляра класса
AdminService& AdminService::getInstance() {
    static AdminService instance;
    return instance;
}

// Проверка, является ли пользователь администратором
bool AdminService::isAdmin(int userId) {
    try {
        auto result = Database::getInstance().executeQuery("get_user_by_id", {to_string(userId)});
        if (!result.empty()) {
            return result[0]["is_admin"].as<bool>();
        }
    } catch (const exception& e) {
        // Ошибка при проверке - не администратор
    }
    return false;
}

// Создание нового мероприятия
json AdminService::createEvent(int adminId, const string& title, const string& genre,
                                int venueId, const string& eventDate, const string& ageRestriction,
                                const string& description, const vector<TicketCategoryCreate>& categories) {
    json response;
    
    // Проверка прав администратора
    if (!isAdmin(adminId)) {
        response["success"] = false;
        response["error"] = "Недостаточно прав для создания мероприятия";
        return response;
    }
    
    if (categories.empty()) {
        response["success"] = false;
        response["error"] = "Необходимо указать хотя бы одну категорию билетов";
        return response;
    }
    
    try {
        // Получаем вместимость площадки
        auto venueResult = Database::getInstance().executeQuery("get_venue_by_id", {to_string(venueId)});
        if (venueResult.empty()) {
            response["success"] = false;
            response["error"] = "Площадка не найдена";
            return response;
        }
        
        int venueCapacity = venueResult[0]["capacity"].as<int>();
        
        // Подсчет общего количества билетов
        int totalTickets = 0;
        for (const auto& category : categories) {
            totalTickets += category.totalSeats;
        }
        
        // Проверка: количество билетов не должно превышать вместимость
        if (totalTickets > venueCapacity) {
            response["success"] = false;
            response["error"] = "Количество продаваемых билетов больше вместимости места проведения мероприятия";
            return response;
        }
        
        // Создание мероприятия
        auto eventResult = Database::getInstance().executeQuery("create_event", 
            {title, genre, to_string(venueId), eventDate, ageRestriction, description, to_string(adminId)});
        
        if (eventResult.empty()) {
        response["success"] = false;
            response["error"] = "Ошибка создания мероприятия";
            return response;
        }
        
        int eventId = eventResult[0]["id"].as<int>();
        
        // Создание категорий билетов и билетов
        for (const auto& category : categories) {
            auto categoryResult = Database::getInstance().executeQuery("create_ticket_category", {
                to_string(eventId),
                category.categoryName,
                to_string(category.price),
                to_string(category.totalSeats),
                to_string(category.rowsCount),
                to_string(category.seatsPerRow)
            });
            
            if (!categoryResult.empty()) {
                int categoryId = categoryResult[0]["id"].as<int>();
                
                // Создание билетов для категории
                createTicketsForCategory(eventId, categoryId, category.rowsCount, 
                                          category.seatsPerRow, category.price, category.totalSeats);
            }
        }
        
        response["success"] = true;
        response["eventId"] = eventId;
        response["message"] = "Мероприятие успешно создано";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка создания мероприятия: " + string(e.what());
    }
    
    return response;
}

// Удаление мероприятия
json AdminService::deleteEvent(int adminId, int eventId) {
    json response;
    
    // Проверка прав администратора
    if (!isAdmin(adminId)) {
        response["success"] = false;
        response["error"] = "Недостаточно прав для удаления мероприятия";
        return response;
    }
    
    try {
        Database::getInstance().executeQuery("delete_event", {to_string(eventId)});
        
        response["success"] = true;
        response["message"] = "Мероприятие успешно удалено";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка удаления мероприятия: " + string(e.what());
    }
    
    return response;
}

// Получение данных мероприятия для редактирования
json AdminService::getEventForEdit(int adminId, int eventId) {
    json response;
    
    // Проверка прав администратора
    if (!isAdmin(adminId)) {
        response["success"] = false;
        response["error"] = "Недостаточно прав для редактирования мероприятия";
        return response;
    }
    
    try {
        // Получение данных мероприятия
        auto eventResult = Database::getInstance().executeQuery("get_event_by_id", {to_string(eventId)});
        
        if (eventResult.empty()) {
            response["success"] = false;
            response["error"] = "Мероприятие не найдено";
            return response;
        }
        
        json eventData;
        eventData["id"] = eventResult[0]["id"].as<int>();
        eventData["title"] = eventResult[0]["title"].as<string>();
        eventData["genre"] = eventResult[0]["genre"].as<string>();
        eventData["eventDate"] = eventResult[0]["event_date"].as<string>();
        eventData["ageRestriction"] = eventResult[0]["age_restriction"].as<string>();
        eventData["description"] = eventResult[0]["description"].is_null() ? "" : eventResult[0]["description"].as<string>();
        eventData["venueId"] = eventResult[0]["venue_id"].as<int>();
        eventData["venueName"] = eventResult[0]["venue_name"].as<string>();
        eventData["venueCity"] = eventResult[0]["venue_city"].as<string>();
        
        // Получение категорий билетов
        auto categoriesResult = Database::getInstance().executeQuery("get_event_categories_for_update", {to_string(eventId)});
        
        json categories = json::array();
        for (const auto& row : categoriesResult) {
            json cat;
            cat["id"] = row["id"].as<int>();
            cat["categoryName"] = row["category_name"].as<string>();
            cat["price"] = row["price"].as<double>();
            cat["totalSeats"] = row["total_seats"].as<int>();
            cat["availableSeats"] = row["available_seats"].as<int>();
            cat["rowsCount"] = row["rows_count"].is_null() ? 0 : row["rows_count"].as<int>();
            cat["seatsPerRow"] = row["seats_per_row"].is_null() ? 0 : row["seats_per_row"].as<int>();
            categories.push_back(cat);
        }
        
        eventData["categories"] = categories;
        
        response["success"] = true;
        response["event"] = eventData;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения данных мероприятия: " + string(e.what());
    }
    
    return response;
}

// Обновление мероприятия
json AdminService::updateEvent(int adminId, int eventId, const string& title, const string& genre,
                                const string& eventDate, const string& ageRestriction,
                                const string& description, const vector<TicketCategoryUpdate>& categories) {
    json response;
    
    // Проверка прав администратора
    if (!isAdmin(adminId)) {
        response["success"] = false;
        response["error"] = "Недостаточно прав для редактирования мероприятия";
        return response;
    }
    
    try {
        // Получение текущих данных мероприятия
        auto eventResult = Database::getInstance().executeQuery("get_event_by_id", {to_string(eventId)});
        
        if (eventResult.empty()) {
            response["success"] = false;
            response["error"] = "Мероприятие не найдено";
            return response;
        }
        
        int venueId = eventResult[0]["venue_id"].as<int>();
        
        // Обновление мероприятия
        Database::getInstance().executeQuery("update_event", 
            {title, genre, to_string(venueId), eventDate, ageRestriction, description, to_string(eventId)});
        
        // Обновление категорий билетов
        for (const auto& category : categories) {
            // Получаем текущие данные категории
            auto catResult = Database::getInstance().executeQuery("get_ticket_categories_by_event", {to_string(eventId)});
            
            for (const auto& row : catResult) {
                if (row["id"].as<int>() == category.categoryId) {
                    int currentAvailable = row["available_seats"].as<int>();
                    int currentTotal = row["total_seats"].as<int>();
                    int soldSeats = currentTotal - currentAvailable;
                    
                    // Новое количество доступных мест
                    int newAvailable = category.totalSeats - soldSeats;
                    if (newAvailable < 0) newAvailable = 0;
                    
                    Database::getInstance().executeQuery("update_ticket_category", 
                        {to_string(category.price), to_string(category.totalSeats), 
                         to_string(newAvailable), to_string(category.categoryId)});
                    break;
                }
            }
        }
        
        response["success"] = true;
        response["message"] = "Мероприятие успешно обновлено";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка обновления мероприятия: " + string(e.what());
    }
    
    return response;
}

// Получение категорий по умолчанию для площадки
json AdminService::getVenueDefaultCategories(int venueId) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_venue_default_categories", {to_string(venueId)});
        
        json categories = json::array();
        for (const auto& row : result) {
            json category;
            category["id"] = row["id"].as<int>();
            category["venueId"] = row["venue_id"].as<int>();
            category["categoryName"] = row["category_name"].as<string>();
            category["rowsCount"] = row["rows_count"].is_null() ? 0 : row["rows_count"].as<int>();
            category["seatsPerRow"] = row["seats_per_row"].is_null() ? 0 : row["seats_per_row"].as<int>();
            category["defaultPrice"] = row["default_price"].is_null() ? 0.0 : row["default_price"].as<double>();
            categories.push_back(category);
        }
        
        response["success"] = true;
        response["categories"] = categories;
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка получения категорий: " + string(e.what());
    }
    
    return response;
}

// Получение площадок по городу
json AdminService::getVenuesByCity(const string& city) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_venues_by_city", {city});
        
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

// Получение всех площадок
json AdminService::getAllVenues() {
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

// Создание билетов для категории
void AdminService::createTicketsForCategory(int eventId, int categoryId, int rowsCount, 
                                             int seatsPerRow, double price, int totalSeats) {
    try {
        if (rowsCount > 0 && seatsPerRow > 0) {
            // Билеты с местами
            for (int row = 1; row <= rowsCount; ++row) {
                for (int seat = 1; seat <= seatsPerRow; ++seat) {
                    Database::getInstance().executeQuery("create_available_ticket", {
                        to_string(eventId),
                        to_string(categoryId),
                        to_string(row),
                        to_string(seat),
                        to_string(price)
                    });
                }
            }
        } else {
            // Билеты без конкретных мест (танцпол, фан-зона)
            for (int i = 1; i <= totalSeats; ++i) {
                Database::getInstance().executeQuery("create_available_ticket", {
                    to_string(eventId),
                    to_string(categoryId),
                    "NULL",
                    to_string(i),
                    to_string(price)
                });
            }
        }
    } catch (const exception& e) {
        // Ошибка при создании билетов
        throw;
    }
}
