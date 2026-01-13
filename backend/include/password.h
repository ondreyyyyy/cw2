/*
 * password.h - Заголовочный файл для работы с паролями
 * Содержит класс PasswordUtils для хеширования и проверки паролей
 */

#ifndef PASSWORD_H
#define PASSWORD_H

#include <string>
#include <stdexcept>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <cstring>

using namespace std;

// Класс для работы с паролями
class PasswordUtils {
public:
    // Хеширование пароля с солью
    static string hashPassword(const string& password);

    // Проверка пароля по хешу
    static bool verifyPassword(const string& password, const string& storedHash);

private:
    // Преобразование байтов в шестнадцатеричную строку
    static string bytesToHex(const unsigned char* bytes, size_t length);

    // Сравнение строк за постоянное время (защита от timing-атак)
    static bool constantTimeCompare(const string& a, const string& b);
};

#endif // PASSWORD_H
