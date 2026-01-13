/*
 * eventservice.h - Заголовочный файл сервиса мероприятий
 * Содержит класс EventService для работы с мероприятиями
 */

#ifndef EVENTSERVICE_H
#define EVENTSERVICE_H

#include <string>
#include <vector>
#include "database.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Класс сервиса мероприятий
class EventService {
public:
    // Получение единственного экземпляра класса (синглтон)
    static EventService& getInstance();

    // Получение всех мероприятий
    json getAllEvents();

    // Получение мероприятия по ID
    json getEventById(int eventId);

    // Получение отфильтрованных мероприятий
    json getFilteredEvents(const string& genre, const string& city,
                           const string& dateFrom, const string& dateTo,
                           const string& venue, const string& ageRestriction);

    // Получение категорий билетов для мероприятия
    json getTicketCategories(int eventId);

    // Получение доступных билетов для мероприятия
    json getAvailableTickets(int eventId, int categoryId = -1);

    // Получение списка жанров
    json getGenres();

    // Получение списка городов
    json getCities();

    // Получение списка площадок
    json getVenues();

private:
    EventService() = default;
    EventService(const EventService&) = delete;
    EventService& operator=(const EventService&) = delete;
};

#endif // EVENTSERVICE_H
