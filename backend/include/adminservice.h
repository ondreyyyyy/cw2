/*
 * adminservice.h - Заголовочный файл сервиса администратора
 * Содержит класс AdminService для управления мероприятиями
 */

#ifndef ADMINSERVICE_H
#define ADMINSERVICE_H

#include <string>
#include <vector>
#include "database.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Структура для создания категории билетов
struct TicketCategoryCreate {
    string categoryName;
    double price;
    int totalSeats;
    int rowsCount;
    int seatsPerRow;
};

// Структура для обновления категории билетов
struct TicketCategoryUpdate {
    int categoryId;
    double price;
    int totalSeats;
};

// Класс сервиса администратора
class AdminService {
public:
    // Получение единственного экземпляра класса (синглтон)
    static AdminService& getInstance();

    // Проверка, является ли пользователь администратором
    bool isAdmin(int userId);

    // Создание нового мероприятия
    json createEvent(int adminId, const string& title, const string& genre,
                     int venueId, const string& eventDate, const string& ageRestriction,
                     const string& description, const vector<TicketCategoryCreate>& categories);

    // Удаление мероприятия
    json deleteEvent(int adminId, int eventId);

    // Обновление мероприятия
    json updateEvent(int adminId, int eventId, const string& title, const string& genre,
                     const string& eventDate, const string& ageRestriction,
                     const string& description, const vector<TicketCategoryUpdate>& categories);

    // Получение данных мероприятия для редактирования
    json getEventForEdit(int adminId, int eventId);

    // Получение категорий по умолчанию для площадки
    json getVenueDefaultCategories(int venueId);

    // Получение площадок по городу
    json getVenuesByCity(const string& city);

    // Получение всех площадок
    json getAllVenues();

private:
    AdminService() = default;
    AdminService(const AdminService&) = delete;
    AdminService& operator=(const AdminService&) = delete;

    // Создание билетов для категории
    void createTicketsForCategory(int eventId, int categoryId, int rowsCount, 
                                   int seatsPerRow, double price, int totalSeats);
};

#endif // ADMINSERVICE_H
