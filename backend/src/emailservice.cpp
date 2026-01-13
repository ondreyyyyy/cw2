/*
 * emailservice.cpp - Реализация класса EmailService
 * Сервис отправки email сообщений через SMTP с поддержкой TLS
 */

#include "emailservice.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

// Получение единственного экземпляра класса
EmailService& EmailService::getInstance() {
    static EmailService instance;
    return instance;
}

// Генерация кода подтверждения (6 цифр)
string EmailService::generateVerificationCode() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(100000, 999999);
    return to_string(dis(gen));
}

// Base64 кодирование для SMTP аутентификации
string base64Encode(const string& input) {
    static const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string result;
    int val = 0, valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        result.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (result.size() % 4) {
        result.push_back('=');
    }
    
    return result;
}

// Чтение ответа SMTP сервера
string readSmtpResponse(SSL* ssl) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    SSL_read(ssl, buffer, sizeof(buffer) - 1);
    return string(buffer);
}

// Чтение ответа от сокета без SSL
string readSocketResponse(int sock) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    return string(buffer);
}

// Отправка кода подтверждения на email
json EmailService::sendVerificationCode(const string& email) {
    json response;
    
    try {
        // Генерация кода
        string code = generateVerificationCode();
        
        // Время истечения кода (10 минут)
        auto now = chrono::system_clock::now();
        auto expiresAt = now + chrono::minutes(10);
        auto expiresTime = chrono::system_clock::to_time_t(expiresAt);
        
        // Форматирование времени для PostgreSQL
        stringstream ss;
        ss << put_time(gmtime(&expiresTime), "%Y-%m-%d %H:%M:%S");
        string expiresStr = ss.str();
        
        // Сохранение кода в базе данных
        Database::getInstance().executeQuery("create_verification_code", {email, code, expiresStr});
        
        // Отправка email
        string subject = "Код подтверждения tickethub";
        string body = "Здравствуйте!\n\nВаш код подтверждения для регистрации в системе tickethub: " + code + "\n\nКод действителен 10 минут.\n\nЕсли вы не запрашивали код подтверждения, проигнорируйте это письмо.\n\nС уважением,\nКоманда tickethub";
        
        bool emailSent = sendEmail(email, subject, body);
        
        if (emailSent) {
            response["success"] = true;
            response["message"] = "Код подтверждения отправлен на указанную почту";
        } else {
            response["success"] = false;
            response["error"] = "Не удалось отправить email. Проверьте правильность почты.";
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка отправки кода: " + string(e.what());
    }
    
    return response;
}

// Проверка кода подтверждения
json EmailService::verifyCode(const string& email, const string& code) {
    json response;
    
    try {
        auto result = Database::getInstance().executeQuery("get_verification_code", {email, code});
        
        if (result.empty()) {
            response["success"] = false;
            response["error"] = "Неверный код подтверждения или код истек";
            return response;
        }
        
        int codeId = result[0]["id"].as<int>();
        
        // Отмечаем код как использованный
        Database::getInstance().executeQuery("mark_code_used", {to_string(codeId)});
        
        response["success"] = true;
        response["message"] = "Код подтвержден";
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка проверки кода: " + string(e.what());
    }
    
    return response;
}

// Отправка пароля на email (для восстановления)
json EmailService::sendPasswordToEmail(const string& email, const string& password) {
    json response;
    
    try {
        string subject = "Восстановление пароля tickethub";
        string body = "Здравствуйте!\n\nВаш новый пароль для входа в систему tickethub: " + password + "\n\nРекомендуем сменить пароль после входа в личный кабинет.\n\nЕсли вы не запрашивали восстановление пароля, срочно обратитесь в службу поддержки.\n\nС уважением,\nКоманда tickethub";
        
        bool emailSent = sendEmail(email, subject, body);
        
        if (emailSent) {
            response["success"] = true;
            response["message"] = "Новый пароль отправлен на указанную почту";
        } else {
            response["success"] = false;
            response["error"] = "Не удалось отправить email. Проверьте правильность почты.";
        }
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = "Ошибка отправки пароля: " + string(e.what());
    }
    
    return response;
}

// Отправка email через SMTP с поддержкой SSL (порт 465) и STARTTLS (порт 587)
bool EmailService::sendEmail(const string& to, const string& subject, const string& body) {
    // Получаем настройки SMTP из конфигурации
    string smtpServer = Config::getInstance().get("SMTP_SERVER", "smtp.gmail.com");
    string smtpPortStr = Config::getInstance().get("SMTP_PORT", "587");
    string smtpUser = Config::getInstance().get("SMTP_USER", "");
    string smtpPassword = Config::getInstance().get("SMTP_PASSWORD", "");
    string smtpSsl = Config::getInstance().get("SMTP_SSL", "false");
    
    // Проверяем наличие настроек
    if (smtpUser.empty() || smtpPassword.empty()) {
        // SMTP не настроен
        return false;
    }
    
    int smtpPort = stoi(smtpPortStr);
    bool useDirectSsl = (smtpSsl == "true" || smtpPort == 465);
    int sock = -1;
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    
    try {
        // Инициализация OpenSSL
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        
        // Разрешаем имя сервера
        struct hostent* host = gethostbyname(smtpServer.c_str());
        if (!host) {
            throw runtime_error("Не удалось разрешить имя SMTP сервера");
        }
        
        // Создаем сокет
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw runtime_error("Не удалось создать сокет");
        }
        
        // Настройка адреса сервера
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(smtpPort);
        memcpy(&serverAddr.sin_addr.s_addr, host->h_addr, host->h_length);
        
        // Подключаемся к серверу
        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            throw runtime_error("Не удалось подключиться к SMTP серверу");
        }
        
        // Создаем SSL контекст
        ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            throw runtime_error("Не удалось создать SSL контекст");
        }
        
        string response;
        
        if (useDirectSsl) {
            // Прямое SSL подключение (порт 465)
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, sock);
            
            if (SSL_connect(ssl) <= 0) {
                throw runtime_error("Не удалось установить SSL соединение");
            }
            
            // Читаем приветствие сервера
            response = readSmtpResponse(ssl);
            if (response.substr(0, 3) != "220") {
                throw runtime_error("Неверный ответ сервера: " + response);
            }
            
            // Отправляем EHLO
            string ehlo = "EHLO localhost\r\n";
            SSL_write(ssl, ehlo.c_str(), ehlo.length());
            response = readSmtpResponse(ssl);
        } else {
            // STARTTLS подключение (порт 587)
            // Читаем приветствие сервера
            response = readSocketResponse(sock);
            if (response.substr(0, 3) != "220") {
                throw runtime_error("Неверный ответ сервера: " + response);
            }
            
            // Отправляем EHLO
            string ehlo = "EHLO localhost\r\n";
            send(sock, ehlo.c_str(), ehlo.length(), 0);
            response = readSocketResponse(sock);
            
            // Отправляем STARTTLS для шифрования
            string starttls = "STARTTLS\r\n";
            send(sock, starttls.c_str(), starttls.length(), 0);
            response = readSocketResponse(sock);
            if (response.substr(0, 3) != "220") {
                throw runtime_error("Сервер не поддерживает STARTTLS");
            }
            
            // Создаем SSL соединение
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, sock);
            
            if (SSL_connect(ssl) <= 0) {
                throw runtime_error("Не удалось установить SSL соединение");
            }
            
            // Повторяем EHLO после STARTTLS
            ehlo = "EHLO localhost\r\n";
            SSL_write(ssl, ehlo.c_str(), ehlo.length());
            response = readSmtpResponse(ssl);
        }
        
        // Аутентификация AUTH LOGIN
        string authLogin = "AUTH LOGIN\r\n";
        SSL_write(ssl, authLogin.c_str(), authLogin.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "334") {
            throw runtime_error("Ошибка аутентификации: " + response);
        }
        
        // Отправляем логин в Base64
        string userB64 = base64Encode(smtpUser) + "\r\n";
        SSL_write(ssl, userB64.c_str(), userB64.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "334") {
            throw runtime_error("Ошибка аутентификации (логин): " + response);
        }
        
        // Отправляем пароль в Base64
        string passB64 = base64Encode(smtpPassword) + "\r\n";
        SSL_write(ssl, passB64.c_str(), passB64.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "235") {
            throw runtime_error("Ошибка аутентификации (пароль): " + response);
        }
        
        // MAIL FROM
        string mailFrom = "MAIL FROM:<" + smtpUser + ">\r\n";
        SSL_write(ssl, mailFrom.c_str(), mailFrom.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "250") {
            throw runtime_error("Ошибка MAIL FROM: " + response);
        }
        
        // RCPT TO
        string rcptTo = "RCPT TO:<" + to + ">\r\n";
        SSL_write(ssl, rcptTo.c_str(), rcptTo.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "250") {
            throw runtime_error("Ошибка RCPT TO: " + response);
        }
        
        // DATA
        string data = "DATA\r\n";
        SSL_write(ssl, data.c_str(), data.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "354") {
            throw runtime_error("Ошибка DATA: " + response);
        }
        
        // Формируем письмо
        stringstream emailContent;
        emailContent << "From: tickethub <" << smtpUser << ">\r\n";
        emailContent << "To: <" << to << ">\r\n";
        emailContent << "Subject: =?UTF-8?B?" << base64Encode(subject) << "?=\r\n";
        emailContent << "MIME-Version: 1.0\r\n";
        emailContent << "Content-Type: text/plain; charset=UTF-8\r\n";
        emailContent << "Content-Transfer-Encoding: 8bit\r\n";
        emailContent << "\r\n";
        emailContent << body << "\r\n";
        emailContent << ".\r\n";
        
        string emailStr = emailContent.str();
        SSL_write(ssl, emailStr.c_str(), emailStr.length());
        response = readSmtpResponse(ssl);
        if (response.substr(0, 3) != "250") {
            throw runtime_error("Ошибка отправки письма: " + response);
        }
        
        // QUIT
        string quit = "QUIT\r\n";
        SSL_write(ssl, quit.c_str(), quit.length());
        
        // Освобождаем ресурсы
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sock);
        
        return true;
        
    } catch (const exception& e) {
        // Освобождаем ресурсы при ошибке
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
        if (ctx) {
            SSL_CTX_free(ctx);
        }
        if (sock >= 0) {
            close(sock);
        }
        
        // Логируем ошибку
        return false;
    }
}
