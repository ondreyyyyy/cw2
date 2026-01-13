/*
 * auth.h - Заголовочный файл сервиса авторизации
 * Содержит класс AuthService для регистрации и входа пользователей
 */

#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <optional>
#include "database.h"
#include "password.h"
#include "jwt.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Структура для хранения данных пользователя
struct User {
    int id;
    string login;
    string email;
    string fullName;
    bool isAdmin;
    bool isVerified;
};

// Класс сервиса авторизации
class AuthService {
public:
    // Получение единственного экземпляра класса (синглтон)
    static AuthService& getInstance();

    // Запрос кода подтверждения для регистрации
    json requestVerificationCode(const string& email);

    // Регистрация нового пользователя с кодом подтверждения
    json registerUser(const string& login, const string& email, 
                      const string& fullName, const string& password,
                      const string& verificationCode);

    // Вход пользователя в систему
    json login(const string& login, const string& password);

    // Проверка токена и получение данных пользователя
    optional<User> verifyAndGetUser(const string& token);

    // Восстановление пароля (отправка на почту)
    json recoverPassword(const string& login, const string& email);

    // Проверка существования пользователя
    json verifyUserExists(const string& login, const string& email);

    // Сброс пароля (после подтверждения кода)
    json resetPassword(const string& login, const string& email, const string& newPassword);

    // Смена пароля
    json changePassword(int userId, const string& oldPassword, const string& newPassword);

    // Инициализация пароля администратора
    void initAdminPassword();

private:
    AuthService() = default;
    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;
};

#endif // AUTH_H
