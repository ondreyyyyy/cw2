/*
 * jwt.cpp - Реализация класса JWTUtils
 * Создание и проверка JWT токенов
 */

#include "jwt.h"

using namespace std;
using json = nlohmann::json;

// Создание JWT токена для пользователя
string JWTUtils::createToken(int userId, const string& login) {
    json header = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };

    auto now = chrono::system_clock::now();
    auto exp = now + chrono::hours(24); // Токен действителен 24 часа
    
    json payload = {
        {"userId", userId},
        {"login", login},
        {"iat", chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count()},
        {"exp", chrono::duration_cast<chrono::seconds>(exp.time_since_epoch()).count()}
    };

    string headerEncoded = base64UrlEncode(header.dump());
    string payloadEncoded = base64UrlEncode(payload.dump());
    
    string data = headerEncoded + "." + payloadEncoded;
    string signature = sign(data);
    
    return data + "." + signature;
}

// Проверка JWT токена и извлечение данных
bool JWTUtils::verifyToken(const string& token, json& payload) {
    vector<string> parts = split(token, '.');
    if (parts.size() != 3) {
        return false;
    }

    string data = parts[0] + "." + parts[1];
    string expectedSignature = sign(data);
    
    if (!constantTimeCompare(parts[2], expectedSignature)) {
        return false;
    }

    try {
        payload = json::parse(base64UrlDecode(parts[1]));
        
        // Проверка срока действия
        auto now = chrono::system_clock::now();
        auto nowSeconds = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();
        
        if (payload.contains("exp") && payload["exp"].get<long long>() < nowSeconds) {
            return false;
        }
        
        return true;
    } catch (const exception& e) {
        return false;
    }
}

// Получение ID пользователя из токена
int JWTUtils::getUserIdFromToken(const string& token) {
    json payload;
    if (verifyToken(token, payload)) {
        return payload["userId"].get<int>();
    }
    return -1;
}

// Получение логина пользователя из токена
string JWTUtils::getLoginFromToken(const string& token) {
    json payload;
    if (verifyToken(token, payload)) {
        return payload["login"].get<string>();
    }
    return "";
}

// Кодирование в Base64 URL-safe формат
string JWTUtils::base64UrlEncode(const string& input) {
    static const char* base64Chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    string output;
    int val = 0, valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        output.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    // Преобразование в URL-safe Base64
    for (char& c : output) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    
    // Удаление padding
    while (!output.empty() && output.back() == '=') {
        output.pop_back();
    }
    
    return output;
}

// Декодирование из Base64 URL-safe формата
string JWTUtils::base64UrlDecode(const string& input) {
    string base64 = input;
    
    // Преобразование из URL-safe Base64
    for (char& c : base64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    
    // Добавление padding
    while (base64.length() % 4 != 0) {
        base64 += '=';
    }
    
    static const string base64Chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    string output;
    int val = 0, valb = -8;
    
    for (char c : base64) {
        if (c == '=') break;
        size_t pos = base64Chars.find(c);
        if (pos == string::npos) continue;
        val = (val << 6) + static_cast<int>(pos);
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return output;
}

// Подпись данных с помощью HMAC-SHA256
string JWTUtils::sign(const string& data) {
    string secret = Config::getInstance().get("JWT_SECRET");
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    HMAC(EVP_sha256(), 
         secret.c_str(), static_cast<int>(secret.length()),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         hash, &hashLen);
    
    return base64UrlEncode(string(reinterpret_cast<char*>(hash), hashLen));
}

// Разделение строки по разделителю
vector<string> JWTUtils::split(const string& str, char delimiter) {
    vector<string> parts;
    string part;
    for (char c : str) {
        if (c == delimiter) {
            parts.push_back(part);
            part.clear();
        } else {
            part += c;
        }
    }
    parts.push_back(part);
    return parts;
}

// Сравнение строк за постоянное время
bool JWTUtils::constantTimeCompare(const string& a, const string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    
    unsigned char result = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}
