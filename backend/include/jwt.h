/*
 * jwt.h - Заголовочный файл для работы с JWT токенами
 * Содержит класс JWTUtils для создания и проверки JWT токенов
 */

#ifndef JWT_H
#define JWT_H

#include <string>
#include <vector>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "json.hpp"
#include "config.h"

using namespace std;
using json = nlohmann::json;

// Класс для работы с JWT токенами
class JWTUtils {
public:
    // Создание JWT токена для пользователя
    static string createToken(int userId, const string& login);

    // Проверка JWT токена и извлечение данных
    static bool verifyToken(const string& token, json& payload);

    // Получение ID пользователя из токена
    static int getUserIdFromToken(const string& token);

    // Получение логина пользователя из токена
    static string getLoginFromToken(const string& token);

private:
    // Кодирование в Base64 URL-safe формат
    static string base64UrlEncode(const string& input);

    // Декодирование из Base64 URL-safe формата
    static string base64UrlDecode(const string& input);

    // Подпись данных с помощью HMAC-SHA256
    static string sign(const string& data);

    // Разделение строки по разделителю
    static vector<string> split(const string& str, char delimiter);

    // Сравнение строк за постоянное время
    static bool constantTimeCompare(const string& a, const string& b);
};

#endif // JWT_H
