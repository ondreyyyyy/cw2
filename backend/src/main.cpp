/*
 * main.cpp - Главный файл приложения
 * Точка входа и настройка маршрутов API
 */

#include <iostream>
#include "config.h"
#include "queryloader.h"
#include "database.h"
#include "auth.h"
#include "eventservice.h"
#include "ticketservice.h"
#include "adminservice.h"
#include "emailservice.h"
#include "httpserver.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Проверка авторизации и получение ID пользователя
bool checkAuth(const HttpRequest& request, HttpResponse& response, int& userId) {
    string token = request.getAuthToken();
    if (token.empty()) {
        response.statusCode = 401;
        json error = {{"success", false}, {"error", "Требуется авторизация"}};
        response.setJson(error);
        return false;
    }
    
    auto user = AuthService::getInstance().verifyAndGetUser(token);
    if (!user) {
        response.statusCode = 401;
        json error = {{"success", false}, {"error", "Неверный токен авторизации"}};
        response.setJson(error);
        return false;
    }
    
    userId = user->id;
    return true;
}

// Проверка прав администратора
bool checkAdmin(const HttpRequest& request, HttpResponse& response, int& userId) {
    if (!checkAuth(request, response, userId)) {
        return false;
    }
    
    if (!AdminService::getInstance().isAdmin(userId)) {
        response.statusCode = 403;
        json error = {{"success", false}, {"error", "Недостаточно прав"}};
        response.setJson(error);
        return false;
    }
    
    return true;
}

int main() {
    try {
        // Загрузка конфигурации
        Config::getInstance().loadFromFile("_env");
        cout << "Конфигурация загружена" << endl;
        
        // Загрузка SQL запросов
        QueryLoader::getInstance().loadFromFile("sql/queries.sql");
        cout << "SQL запросы загружены" << endl;
        
        // Инициализация базы данных
        Database::getInstance().initialize();
        cout << "Подключение к базе данных установлено" << endl;
        
        // Инициализация пароля администратора
        AuthService::getInstance().initAdminPassword();
        cout << "Пароль администратора инициализирован" << endl;
        
        // Получение порта из конфигурации
        int port = Config::getInstance().getInt("SERVER_PORT", 8080);
        
        // Создание HTTP сервера
        HttpServer server(port);
        
        // Установка пути к статическим файлам
        server.setStaticPath("../frontend");
        
        // =====================================================
        // API маршруты для авторизации
        // =====================================================
        
        // POST /api/send-verification-code - Отправка кода подтверждения
        server.post("/api/send-verification-code", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                string email = body.value("email", "");
                
                if (email.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Email обязателен"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().requestVerificationCode(email);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // POST /api/register - Регистрация пользователя
        server.post("/api/register", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                
                string login = body.value("login", "");
                string email = body.value("email", "");
                string fullName = body.value("fullName", "");
                string password = body.value("password", "");
                string verificationCode = body.value("verificationCode", "");
                
                if (login.empty() || email.empty() || fullName.empty() || password.empty() || verificationCode.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Все поля обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().registerUser(login, email, fullName, password, verificationCode);
                response.statusCode = result["success"].get<bool>() ? 201 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // POST /api/login - Вход в систему
        server.post("/api/login", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                
                string login = body.value("login", "");
                string password = body.value("password", "");
                
                if (login.empty() || password.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Логин и пароль обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().login(login, password);
                response.statusCode = result["success"].get<bool>() ? 200 : 401;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // GET /api/me - Получение данных текущего пользователя
        server.get("/api/me", [](const HttpRequest& request, HttpResponse& response) {
            string token = request.getAuthToken();
            if (token.empty()) {
                response.statusCode = 401;
                json error = {{"success", false}, {"error", "Требуется авторизация"}};
                response.setJson(error);
                return;
            }
            
            auto user = AuthService::getInstance().verifyAndGetUser(token);
            if (!user) {
                response.statusCode = 401;
                json error = {{"success", false}, {"error", "Неверный токен авторизации"}};
                response.setJson(error);
                return;
            }
            
            json result = {
                {"success", true},
                {"user", {
                    {"id", user->id},
                    {"login", user->login},
                    {"email", user->email},
                    {"fullName", user->fullName},
                    {"isAdmin", user->isAdmin}
                }}
            };
            response.setJson(result);
        });
        
        // POST /api/recover-password - Восстановление пароля
        server.post("/api/recover-password", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                
                string login = body.value("login", "");
                string email = body.value("email", "");
                
                if (login.empty() || email.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Логин и почта обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().recoverPassword(login, email);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // POST /api/change-password - Смена пароля
        server.post("/api/change-password", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAuth(request, response, userId)) return;
            
            try {
                json body = json::parse(request.body);
                
                string oldPassword = body.value("oldPassword", "");
                string newPassword = body.value("newPassword", "");
                
                if (oldPassword.empty() || newPassword.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Все поля обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().changePassword(userId, oldPassword, newPassword);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // POST /api/verify-code - Проверка кода подтверждения
        server.post("/api/verify-code", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                
                string email = body.value("email", "");
                string code = body.value("code", "");
                
                if (email.empty() || code.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Email и код обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = EmailService::getInstance().verifyCode(email, code);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // POST /api/verify-user-exists - Проверка существования пользователя
        server.post("/api/verify-user-exists", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                
                string login = body.value("login", "");
                string email = body.value("email", "");
                
                if (login.empty() || email.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Логин и почта обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().verifyUserExists(login, email);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // POST /api/reset-password - Сброс пароля после подтверждения кода
        server.post("/api/reset-password", [](const HttpRequest& request, HttpResponse& response) {
            try {
                json body = json::parse(request.body);
                
                string login = body.value("login", "");
                string email = body.value("email", "");
                string newPassword = body.value("newPassword", "");
                
                if (login.empty() || email.empty() || newPassword.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Все поля обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = AuthService::getInstance().resetPassword(login, email, newPassword);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // =====================================================
        // API маршруты для мероприятий
        // =====================================================
        
        // GET /api/events - Получение списка мероприятий
        server.get("/api/events", [](const HttpRequest& request, HttpResponse& response) {
            string genre = request.getQueryParam("genre");
            string city = request.getQueryParam("city");
            string dateFrom = request.getQueryParam("dateFrom");
            string dateTo = request.getQueryParam("dateTo");
            string venue = request.getQueryParam("venue");
            string ageRestriction = request.getQueryParam("ageRestriction");
            
            json result;
            if (genre.empty() && city.empty() && dateFrom.empty() && 
                dateTo.empty() && venue.empty() && ageRestriction.empty()) {
                result = EventService::getInstance().getAllEvents();
            } else {
                result = EventService::getInstance().getFilteredEvents(
                    genre, city, dateFrom, dateTo, venue, ageRestriction);
            }
            
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/event - Получение информации о мероприятии
        server.get("/api/event", [](const HttpRequest& request, HttpResponse& response) {
            string idStr = request.getQueryParam("id");
            if (idStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID мероприятия обязателен"}};
                response.setJson(error);
                return;
            }
            
            int eventId;
            try {
                eventId = stoi(idStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID мероприятия"}};
                response.setJson(error);
                return;
            }
            
            json result = EventService::getInstance().getEventById(eventId);
            response.statusCode = result["success"].get<bool>() ? 200 : 404;
            response.setJson(result);
        });
        
        // GET /api/genres - Получение списка жанров
        server.get("/api/genres", [](const HttpRequest& request, HttpResponse& response) {
            json result = EventService::getInstance().getGenres();
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/cities - Получение списка городов
        server.get("/api/cities", [](const HttpRequest& request, HttpResponse& response) {
            json result = EventService::getInstance().getCities();
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/venues - Получение списка площадок
        server.get("/api/venues", [](const HttpRequest& request, HttpResponse& response) {
            json result = EventService::getInstance().getVenues();
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/categories - Получение категорий билетов для мероприятия
        server.get("/api/categories", [](const HttpRequest& request, HttpResponse& response) {
            string idStr = request.getQueryParam("eventId");
            if (idStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID мероприятия обязателен"}};
                response.setJson(error);
                return;
            }
            
            int eventId;
            try {
                eventId = stoi(idStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID мероприятия"}};
                response.setJson(error);
                return;
            }
            
            json result = EventService::getInstance().getTicketCategories(eventId);
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/tickets/available - Получение доступных билетов
        server.get("/api/tickets/available", [](const HttpRequest& request, HttpResponse& response) {
            string eventIdStr = request.getQueryParam("eventId");
            string categoryIdStr = request.getQueryParam("categoryId");
            
            if (eventIdStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID мероприятия обязателен"}};
                response.setJson(error);
                return;
            }
            
            int eventId;
            int categoryId = -1;
            try {
                eventId = stoi(eventIdStr);
                if (!categoryIdStr.empty()) {
                    categoryId = stoi(categoryIdStr);
                }
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID"}};
                response.setJson(error);
                return;
            }
            
            json result = EventService::getInstance().getAvailableTickets(eventId, categoryId);
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // =====================================================
        // API маршруты для бронирования
        // =====================================================
        
        // POST /api/book - Бронирование билета
        server.post("/api/book", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAuth(request, response, userId)) return;
            
            try {
                json body = json::parse(request.body);
                
                int ticketId = body.value("ticketId", 0);
                int eventId = body.value("eventId", 0);
                
                if (ticketId <= 0 || eventId <= 0) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "ID билета и мероприятия обязательны"}};
                    response.setJson(error);
                    return;
                }
                
                json result = TicketService::getInstance().bookTicket(userId, ticketId, eventId);
                response.statusCode = result["success"].get<bool>() ? 201 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // DELETE /api/book - Отмена бронирования
        server.del("/api/book", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAuth(request, response, userId)) return;
            
            string bookingIdStr = request.getQueryParam("bookingId");
            if (bookingIdStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID бронирования обязателен"}};
                response.setJson(error);
                return;
            }
            
            int bookingId;
            try {
                bookingId = stoi(bookingIdStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID бронирования"}};
                response.setJson(error);
                return;
            }
            
            json result = TicketService::getInstance().cancelBooking(userId, bookingId);
            response.statusCode = result["success"].get<bool>() ? 200 : 400;
            response.setJson(result);
        });
        
        // GET /api/bookings - Получение бронирований пользователя
        server.get("/api/bookings", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAuth(request, response, userId)) return;
            
            string genre = request.getQueryParam("genre");
            string city = request.getQueryParam("city");
            string dateFrom = request.getQueryParam("dateFrom");
            string dateTo = request.getQueryParam("dateTo");
            string venue = request.getQueryParam("venue");
            string ageRestriction = request.getQueryParam("ageRestriction");
            
            json result;
            if (genre.empty() && city.empty() && dateFrom.empty() && 
                dateTo.empty() && venue.empty() && ageRestriction.empty()) {
                result = TicketService::getInstance().getUserBookings(userId);
            } else {
                result = TicketService::getInstance().getFilteredUserBookings(
                    userId, genre, city, dateFrom, dateTo, venue, ageRestriction);
            }
            
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/booking - Получение деталей бронирования
        server.get("/api/booking", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAuth(request, response, userId)) return;
            
            string bookingIdStr = request.getQueryParam("id");
            if (bookingIdStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID бронирования обязателен"}};
                response.setJson(error);
                return;
            }
            
            int bookingId;
            try {
                bookingId = stoi(bookingIdStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID бронирования"}};
                response.setJson(error);
                return;
            }
            
            json result = TicketService::getInstance().getBookingDetails(userId, bookingId);
            response.statusCode = result["success"].get<bool>() ? 200 : 404;
            response.setJson(result);
        });
        
        // =====================================================
        // API маршруты для администратора
        // =====================================================
        
        // GET /api/admin/venues - Получение площадок по городу
        server.get("/api/admin/venues", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAdmin(request, response, userId)) return;
            
            string city = request.getQueryParam("city");
            
            json result;
            if (city.empty()) {
                result = AdminService::getInstance().getAllVenues();
            } else {
                result = AdminService::getInstance().getVenuesByCity(city);
            }
            
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // GET /api/admin/venue-categories - Получение категорий для площадки
        server.get("/api/admin/venue-categories", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAdmin(request, response, userId)) return;
            
            string venueIdStr = request.getQueryParam("venueId");
            if (venueIdStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID площадки обязателен"}};
                response.setJson(error);
                return;
            }
            
            int venueId;
            try {
                venueId = stoi(venueIdStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID площадки"}};
                response.setJson(error);
                return;
            }
            
            json result = AdminService::getInstance().getVenueDefaultCategories(venueId);
            response.statusCode = result["success"].get<bool>() ? 200 : 500;
            response.setJson(result);
        });
        
        // POST /api/admin/event - Создание мероприятия
        server.post("/api/admin/event", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAdmin(request, response, userId)) return;
            
            try {
                json body = json::parse(request.body);
                
                string title = body.value("title", "");
                string genre = body.value("genre", "");
                int venueId = body.value("venueId", 0);
                string eventDate = body.value("eventDate", "");
                string ageRestriction = body.value("ageRestriction", "0+");
                string description = body.value("description", "");
                
                if (title.empty() || genre.empty() || venueId <= 0 || eventDate.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Все обязательные поля должны быть заполнены"}};
                    response.setJson(error);
                    return;
                }
                
                // Парсинг категорий
                vector<TicketCategoryCreate> categories;
                if (body.contains("categories") && body["categories"].is_array()) {
                    for (const auto& cat : body["categories"]) {
                        TicketCategoryCreate category;
                        category.categoryName = cat.value("categoryName", "");
                        category.price = cat.value("price", 0.0);
                        category.totalSeats = cat.value("totalSeats", 0);
                        category.rowsCount = cat.value("rowsCount", 0);
                        category.seatsPerRow = cat.value("seatsPerRow", 0);
                        
                        if (!category.categoryName.empty() && category.totalSeats > 0) {
                            categories.push_back(category);
                        }
                    }
                }
                
                json result = AdminService::getInstance().createEvent(
                    userId, title, genre, venueId, eventDate, ageRestriction, description, categories);
                response.statusCode = result["success"].get<bool>() ? 201 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // DELETE /api/admin/event - Удаление мероприятия
        server.del("/api/admin/event", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAdmin(request, response, userId)) return;
            
            string eventIdStr = request.getQueryParam("id");
            if (eventIdStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID мероприятия обязателен"}};
                response.setJson(error);
                return;
            }
            
            int eventId;
            try {
                eventId = stoi(eventIdStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID мероприятия"}};
                response.setJson(error);
                return;
            }
            
            json result = AdminService::getInstance().deleteEvent(userId, eventId);
            response.statusCode = result["success"].get<bool>() ? 200 : 400;
            response.setJson(result);
        });
        
        // GET /api/admin/event - Получение данных мероприятия для редактирования
        server.get("/api/admin/event", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAdmin(request, response, userId)) return;
            
            string eventIdStr = request.getQueryParam("id");
            if (eventIdStr.empty()) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "ID мероприятия обязателен"}};
                response.setJson(error);
                return;
            }
            
            int eventId;
            try {
                eventId = stoi(eventIdStr);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный ID мероприятия"}};
                response.setJson(error);
                return;
            }
            
            json result = AdminService::getInstance().getEventForEdit(userId, eventId);
            response.statusCode = result["success"].get<bool>() ? 200 : 404;
            response.setJson(result);
        });
        
        // PUT /api/admin/event - Обновление мероприятия
        server.put("/api/admin/event", [](const HttpRequest& request, HttpResponse& response) {
            int userId;
            if (!checkAdmin(request, response, userId)) return;
            
            try {
                json body = json::parse(request.body);
                
                int eventId = body.value("eventId", 0);
                string title = body.value("title", "");
                string genre = body.value("genre", "");
                string eventDate = body.value("eventDate", "");
                string ageRestriction = body.value("ageRestriction", "0+");
                string description = body.value("description", "");
                
                if (eventId <= 0 || title.empty() || genre.empty() || eventDate.empty()) {
                    response.statusCode = 400;
                    json error = {{"success", false}, {"error", "Все обязательные поля должны быть заполнены"}};
                    response.setJson(error);
                    return;
                }
                
                // Парсинг категорий для обновления
                vector<TicketCategoryUpdate> categories;
                if (body.contains("categories") && body["categories"].is_array()) {
                    for (const auto& cat : body["categories"]) {
                        TicketCategoryUpdate category;
                        category.categoryId = cat.value("categoryId", 0);
                        category.price = cat.value("price", 0.0);
                        category.totalSeats = cat.value("totalSeats", 0);
                        
                        if (category.categoryId > 0) {
                            categories.push_back(category);
                        }
                    }
                }
                
                json result = AdminService::getInstance().updateEvent(
                    userId, eventId, title, genre, eventDate, ageRestriction, description, categories);
                response.statusCode = result["success"].get<bool>() ? 200 : 400;
                response.setJson(result);
            } catch (const exception& e) {
                response.statusCode = 400;
                json error = {{"success", false}, {"error", "Неверный формат запроса"}};
                response.setJson(error);
            }
        });
        
        // Запуск сервера
        cout << "Запуск сервера на порту " << port << "..." << endl;
        server.start();
        
    } catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
