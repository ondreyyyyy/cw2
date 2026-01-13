/*
 * password.cpp - Реализация класса PasswordUtils
 * Хеширование и проверка паролей
 */

#include "password.h"

using namespace std;

// Хеширование пароля с солью
string PasswordUtils::hashPassword(const string& password) {
    // Генерация случайной соли
    unsigned char salt[16];
    int rc = RAND_bytes(salt, sizeof(salt));
    if (rc != 1) {
        throw runtime_error("Не удалось сгенерировать безопасные случайные байты для соли");
    }
    
    // Преобразование соли в шестнадцатеричную строку
    string saltHex = bytesToHex(salt, sizeof(salt));
    
    // Хеширование пароля с солью
    string saltedPassword = saltHex + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(saltedPassword.c_str()), 
           saltedPassword.length(), hash);
    
    // Возврат в формате соль:хеш
    return saltHex + ":" + bytesToHex(hash, SHA256_DIGEST_LENGTH);
}

// Проверка пароля по хешу
bool PasswordUtils::verifyPassword(const string& password, const string& storedHash) {
    // Разделение хеша на соль и хеш
    size_t colonPos = storedHash.find(':');
    if (colonPos == string::npos) {
        return false;
    }
    
    string salt = storedHash.substr(0, colonPos);
    string expectedHash = storedHash.substr(colonPos + 1);
    
    // Хеширование пароля с сохраненной солью
    string saltedPassword = salt + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(saltedPassword.c_str()), 
           saltedPassword.length(), hash);
    
    string actualHash = bytesToHex(hash, SHA256_DIGEST_LENGTH);
    
    // Сравнение за постоянное время для защиты от timing-атак
    return constantTimeCompare(expectedHash, actualHash);
}

// Преобразование байтов в шестнадцатеричную строку
string PasswordUtils::bytesToHex(const unsigned char* bytes, size_t length) {
    stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

// Сравнение строк за постоянное время
bool PasswordUtils::constantTimeCompare(const string& a, const string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    
    unsigned char result = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}
