/*
 * emailservice.h - Заголовочный файл сервиса отправки email
 * Содержит класс EmailService для отправки писем
 */

#ifndef EMAILSERVICE_H
#define EMAILSERVICE_H

#include <string>
#include <random>
#include "database.h"
#include "config.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Класс сервиса отправки email
class EmailService {
public:
    // Получение единственного экземпляра класса (синглтон)
    static EmailService& getInstance();

    // Генерация кода подтверждения
    string generateVerificationCode();

    // Отправка кода подтверждения на email
    json sendVerificationCode(const string& email);

    // Проверка кода подтверждения
    json verifyCode(const string& email, const string& code);

    // Отправка пароля на email (для восстановления)
    json sendPasswordToEmail(const string& email, const string& password);

private:
    EmailService() = default;
    EmailService(const EmailService&) = delete;
    EmailService& operator=(const EmailService&) = delete;

    // Отправка email через SMTP
    bool sendEmail(const string& to, const string& subject, const string& body);
};

#endif // EMAILSERVICE_H
