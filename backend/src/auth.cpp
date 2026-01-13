/*
 * auth.cpp - Реализация класса AuthService
 * Сервис авторизации пользователей
 */

#include "auth.h"
#include "emailservice.h"
#include <random>

using namespace std;
using json = nlohmann::json;

// Получение единственного экземпляра класса
AuthService& AuthService::getInstance() {
    static AuthService instance;
    return instance;
}

// Запрос кода подтверждения для регистрации
json AuthService::requestVerificationCode(const string& email) {
    json response;
    
    // Проверка, существует ли email
    try {
        auto result = Database::getInstance().executeQuery("check_email_exists", {email});
        if (!result.empty() && result[0]["exists"].as<bool>()) {
            response["success"] = false;
            response["error"] = "Такой пользователь уже существует";
            return response;
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка проверки почты";
        return response;
    }
    
    // Отправка кода подтверждения
    return EmailService::getInstance().sendVerificationCode(email);
}

// Регистрация нового пользователя с кодом подтверждения
json AuthService::registerUser(const string& login, const string& email, 
                                const string& fullName, const string& password,
                                const string& verificationCode) {
    json response;
    
    // Проверка, существует ли логин
    try {
        auto result = Database::getInstance().executeQuery("check_login_exists", {login});
        if (!result.empty() && result[0]["exists"].as<bool>()) {
            response["success"] = false;
            response["error"] = "Такой пользователь уже существует";
            return response;
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка проверки логина";
        return response;
    }

    // Проверка, существует ли email
    try {
        auto result = Database::getInstance().executeQuery("check_email_exists", {email});
        if (!result.empty() && result[0]["exists"].as<bool>()) {
            response["success"] = false;
            response["error"] = "Такой пользователь уже существует";
            return response;
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка проверки почты";
        return response;
    }

    // Проверка кода подтверждения
    json verifyResult = EmailService::getInstance().verifyCode(email, verificationCode);
    if (!verifyResult["success"].get<bool>()) {
        return verifyResult;
    }

    // Хеширование пароля и создание пользователя
    string passwordHash = PasswordUtils::hashPassword(password);
    
    try {
        auto result = Database::getInstance().executeQuery("create_verified_user", 
            {login, email, fullName, passwordHash});
        
        if (!result.empty()) {
            int userId = result[0]["id"].as<int>();
            string token = JWTUtils::createToken(userId, login);
            
            response["success"] = true;
            response["token"] = token;
            response["user"] = {
                {"id", userId},
                {"login", login},
                {"email", email},
                {"fullName", fullName},
                {"isAdmin", false}
            };
        } else {
            response["success"] = false;
            response["error"] = "Ошибка создания пользователя";
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка регистрации: " + string(e.what());
    }
    
    return response;
}

// Вход пользователя в систему
json AuthService::login(const string& login, const string& password) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_user_by_login", {login});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Неверный логин или пароль";
            return response;
        }

        string storedHash = result[0]["password_hash"].as<string>();
        
        if (!PasswordUtils::verifyPassword(password, storedHash)) {
            response["success"] = false;
            response["error"] = "Неверный логин или пароль";
            return response;
        }

        int userId = result[0]["id"].as<int>();
        string userLogin = result[0]["login"].as<string>();
        string email = result[0]["email"].as<string>();
        string fullName = result[0]["full_name"].as<string>();
        bool isAdmin = result[0]["is_admin"].as<bool>();

        string token = JWTUtils::createToken(userId, userLogin);

        response["success"] = true;
        response["token"] = token;
        response["user"] = {
            {"id", userId},
            {"login", userLogin},
            {"email", email},
            {"fullName", fullName},
            {"isAdmin", isAdmin}
        };
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка входа: " + string(e.what());
    }
    
    return response;
}

// Проверка токена и получение данных пользователя
optional<User> AuthService::verifyAndGetUser(const string& token) {
    json payload;
    if (!JWTUtils::verifyToken(token, payload)) {
        return nullopt;
    }

    try {
        int userId = payload["userId"].get<int>();
        auto result = Database::getInstance().executeQuery("get_user_by_id", {to_string(userId)});
        
        if (result.empty()) {
            return nullopt;
        }

        User user;
        user.id = result[0]["id"].as<int>();
        user.login = result[0]["login"].as<string>();
        user.email = result[0]["email"].as<string>();
        user.fullName = result[0]["full_name"].as<string>();
        user.isAdmin = result[0]["is_admin"].as<bool>();
        user.isVerified = result[0]["is_verified"].as<bool>();
        
        return user;
    } catch (const exception& e) {
        return nullopt;
    }
}

// Восстановление пароля (отправка на почту)
json AuthService::recoverPassword(const string& login, const string& email) {
    json response;
    
    try {
        // Проверка существования пользователя с указанным логином и почтой
        auto result = Database::getInstance().executeQuery("get_user_by_login_and_email", {login, email});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Такого пользователя не существует";
            return response;
        }
        
        int userId = result[0]["id"].as<int>();
        string storedHash = result[0]["password_hash"].as<string>();
        
        // Генерируем безопасный новый пароль
        // Используем комбинацию случайных символов для обеспечения безопасности
        random_device rd;
        mt19937 gen(rd());
        
        const string upperChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const string lowerChars = "abcdefghijklmnopqrstuvwxyz";
        const string digits = "0123456789";
        const string specialChars = "!@#$%^&*";
        
        uniform_int_distribution<> upperDist(0, upperChars.size() - 1);
        uniform_int_distribution<> lowerDist(0, lowerChars.size() - 1);
        uniform_int_distribution<> digitDist(0, digits.size() - 1);
        uniform_int_distribution<> specialDist(0, specialChars.size() - 1);
        
        // Генерируем пароль: 2 заглавные, 4 строчные, 2 цифры, 2 спецсимвола
        string newPassword = "";
        newPassword += upperChars[upperDist(gen)];
        newPassword += upperChars[upperDist(gen)];
        newPassword += lowerChars[lowerDist(gen)];
        newPassword += lowerChars[lowerDist(gen)];
        newPassword += lowerChars[lowerDist(gen)];
        newPassword += lowerChars[lowerDist(gen)];
        newPassword += digits[digitDist(gen)];
        newPassword += digits[digitDist(gen)];
        newPassword += specialChars[specialDist(gen)];
        newPassword += specialChars[specialDist(gen)];
        
        string newHash = PasswordUtils::hashPassword(newPassword);
        
        // Обновляем пароль в базе
        Database::getInstance().executeQuery("update_user_password", {newHash, to_string(userId)});
        
        // Отправляем новый пароль на почту
        return EmailService::getInstance().sendPasswordToEmail(email, newPassword);
        
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка восстановления пароля: " + string(e.what());
    }
    
    return response;
}

// Проверка существования пользователя
json AuthService::verifyUserExists(const string& login, const string& email) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_user_by_login_and_email", {login, email});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Неверный логин или почта";
        } else {
            response["success"] = true;
            response["message"] = "Данные подтверждены";
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка проверки данных";
    }
    
    return response;
}

// Сброс пароля после подтверждения кода
json AuthService::resetPassword(const string& login, const string& email, const string& newPassword) {
    json response;
    
    try {
        // Проверка существования пользователя с указанным логином и почтой
        auto result = Database::getInstance().executeQuery("get_user_by_login_and_email", {login, email});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Ошибка сброса пароля";
            return response;
        }
        
        int userId = result[0]["id"].as<int>();
        
        // Хешируем новый пароль
        string newHash = PasswordUtils::hashPassword(newPassword);
        
        // Обновляем пароль в базе
        Database::getInstance().executeQuery("update_user_password", {newHash, to_string(userId)});
        
        response["success"] = true;
        response["message"] = "Пароль успешно изменен";
        
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка сброса пароля: " + string(e.what());
    }
    
    return response;
}

// Смена пароля
json AuthService::changePassword(int userId, const string& oldPassword, const string& newPassword) {
    json response;
    
    try {
        // Получаем текущий хеш пароля
        auto result = Database::getInstance().executeQuery("get_user_by_login", {to_string(userId)});
        
        // Если не нашли по ID, ищем по логину
        auto userResult = Database::getInstance().executeQuery("get_user_by_id", {to_string(userId)});
        if (userResult.empty()) {
            response["success"] = false;
            response["error"] = "Пользователь не найден";
            return response;
        }
        
        string login = userResult[0]["login"].as<string>();
        auto loginResult = Database::getInstance().executeQuery("get_user_by_login", {login});
        
        if (loginResult.empty()) {
            response["success"] = false;
            response["error"] = "Пользователь не найден";
            return response;
        }
        
        string storedHash = loginResult[0]["password_hash"].as<string>();
        
        // Проверяем старый пароль
        if (!PasswordUtils::verifyPassword(oldPassword, storedHash)) {
            response["success"] = false;
            response["error"] = "Пароль неверный";
            return response;
        }
        
        // Хешируем новый пароль
        string newHash = PasswordUtils::hashPassword(newPassword);
        
        // Обновляем пароль
        Database::getInstance().executeQuery("update_user_password", {newHash, to_string(userId)});
        
        response["success"] = true;
        response["message"] = "Пароль успешно изменен";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка смены пароля: " + string(e.what());
    }
    
    return response;
}

// Инициализация пароля администратора
void AuthService::initAdminPassword() {
    try {
        string adminPassword = "adminpassword";
        string passwordHash = PasswordUtils::hashPassword(adminPassword);
        Database::getInstance().executeQuery("set_admin_password", {passwordHash});
    } catch (const exception& e) {
        // Ошибка инициализации - игнорируем
    }
}
