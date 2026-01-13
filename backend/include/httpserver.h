/*
 * httpserver.h - Заголовочный файл HTTP сервера
 * Содержит классы HttpRequest, HttpResponse и HttpServer
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <map>
#include <set>
#include <functional>
#include <thread>
#include <mutex>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Структура HTTP запроса
struct HttpRequest {
    string method;                      // HTTP метод (GET, POST, и т.д.)
    string path;                        // Путь запроса
    map<string, string> headers;        // Заголовки запроса
    map<string, string> queryParams;    // Параметры запроса
    string body;                        // Тело запроса
    
    // Получение значения заголовка
    string getHeader(const string& name) const;
    
    // Получение параметра запроса
    string getQueryParam(const string& name) const;

    // Получение токена авторизации
    string getAuthToken() const;
};

// Структура HTTP ответа
struct HttpResponse {
    int statusCode = 200;               // Код статуса ответа
    map<string, string> headers;        // Заголовки ответа
    string body;                        // Тело ответа
    
    // Установка JSON тела ответа
    void setJson(const json& data);
    
    // Установка HTML тела ответа
    void setHtml(const string& html);
    
    // Установка CORS заголовков
    void setCors();
};

// Тип обработчика запросов
using RequestHandler = function<void(const HttpRequest&, HttpResponse&)>;

// Класс HTTP сервера
class HttpServer {
public:
    // Конструктор с указанием порта
    HttpServer(int port);
    
    // Регистрация маршрута
    void route(const string& method, const string& path, RequestHandler handler);
    
    // Регистрация GET маршрута
    void get(const string& path, RequestHandler handler);
    
    // Регистрация POST маршрута
    void post(const string& path, RequestHandler handler);
    
    // Регистрация PUT маршрута
    void put(const string& path, RequestHandler handler);
    
    // Регистрация DELETE маршрута
    void del(const string& path, RequestHandler handler);

    // Установка пути к статическим файлам
    void setStaticPath(const string& path);
    
    // Запуск сервера
    void start();
    
    // Остановка сервера
    void stop();

private:
    // Обработка клиентского подключения
    void handleClient(int clientSocket);
    
    // Парсинг HTTP запроса
    HttpRequest parseRequest(const string& raw);
    
    // Парсинг параметров запроса
    void parseQueryParams(const string& query, map<string, string>& params);
    
    // URL декодирование
    string urlDecode(const string& encoded);
    
    // Обслуживание статических файлов
    void serveStaticFile(const string& path, HttpResponse& response);
    
    // Отправка HTTP ответа
    void sendResponse(int socket, const HttpResponse& response);
    
    int serverPort;                          // Порт сервера
    int serverSocket;                        // Сокет сервера
    bool isRunning;                          // Флаг состояния сервера
    string staticFilesPath;                  // Путь к статическим файлам
    map<string, RequestHandler> routes;      // Маршруты
    mutex serverMutex;                       // Мьютекс сервера
};

#endif // HTTPSERVER_H
