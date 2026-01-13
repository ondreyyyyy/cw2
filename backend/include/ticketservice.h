/*
 * ticketservice.h - Заголовочный файл сервиса билетов
 * Содержит класс TicketService для бронирования и управления билетами
 */

#ifndef TICKETSERVICE_H
#define TICKETSERVICE_H

#include <string>
#include <mutex>
#include "database.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Класс сервиса билетов
class TicketService {
public:
    // Получение единственного экземпляра класса (синглтон)
    static TicketService& getInstance();

    // Бронирование билета
    json bookTicket(int userId, int ticketId, int eventId);

    // Отмена бронирования
    json cancelBooking(int userId, int bookingId);

    // Получение бронирований пользователя
    json getUserBookings(int userId);

    // Получение отфильтрованных бронирований пользователя
    json getFilteredUserBookings(int userId, const string& genre, const string& city,
                                  const string& dateFrom, const string& dateTo,
                                  const string& venue, const string& ageRestriction);

    // Получение деталей бронирования
    json getBookingDetails(int userId, int bookingId);

private:
    TicketService() = default;
    TicketService(const TicketService&) = delete;
    TicketService& operator=(const TicketService&) = delete;

    mutex bookingMutex;  // Мьютекс для потокобезопасности бронирования
};

#endif // TICKETSERVICE_H
