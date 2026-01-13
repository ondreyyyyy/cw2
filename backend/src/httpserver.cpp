/*
 * httpserver.cpp - Реализация HTTP сервера
 * Обработка HTTP запросов и маршрутизация
 */

#include "httpserver.h"
#include <iostream>

using namespace std;
using json = nlohmann::json;

// Получение значения заголовка
string HttpRequest::getHeader(const string& name) const {
    auto it = headers.find(name);
    return (it != headers.end()) ? it->second : "";
}

// Получение параметра запроса
string HttpRequest::getQueryParam(const string& name) const {
    auto it = queryParams.find(name);
    return (it != queryParams.end()) ? it->second : "";
}

// Получение токена авторизации
string HttpRequest::getAuthToken() const {
    string auth = getHeader("Authorization");
    if (auth.substr(0, 7) == "Bearer ") {
        return auth.substr(7);
    }
    return "";
}

// Установка JSON тела ответа
void HttpResponse::setJson(const json& data) {
    headers["Content-Type"] = "application/json";
    body = data.dump();
}

// Установка HTML тела ответа
void HttpResponse::setHtml(const string& html) {
    headers["Content-Type"] = "text/html";
    body = html;
}

// Установка CORS заголовков
void HttpResponse::setCors() {
    headers["Access-Control-Allow-Origin"] = "*";
    headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
}

// Конструктор с указанием порта
HttpServer::HttpServer(int port) : serverPort(port), isRunning(false) {}

// Регистрация маршрута
void HttpServer::route(const string& method, const string& path, RequestHandler handler) {
    routes[method + " " + path] = handler;
}

// Регистрация GET маршрута
void HttpServer::get(const string& path, RequestHandler handler) {
    route("GET", path, handler);
}

// Регистрация POST маршрута
void HttpServer::post(const string& path, RequestHandler handler) {
    route("POST", path, handler);
}

// Регистрация PUT маршрута
void HttpServer::put(const string& path, RequestHandler handler) {
    route("PUT", path, handler);
}

// Регистрация DELETE маршрута
void HttpServer::del(const string& path, RequestHandler handler) {
    route("DELETE", path, handler);
}

// Установка пути к статическим файлам
void HttpServer::setStaticPath(const string& path) {
    staticFilesPath = path;
}

// Запуск сервера
void HttpServer::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw runtime_error("Не удалось создать сокет");
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(serverPort);

    if (bind(serverSocket, (sockaddr*)&address, sizeof(address)) < 0) {
        close(serverSocket);
        throw runtime_error("Не удалось привязать сокет к порту " + to_string(serverPort));
    }

    if (listen(serverSocket, 10) < 0) {
        close(serverSocket);
        throw runtime_error("Не удалось начать прослушивание");
    }

    isRunning = true;
    cout << "Сервер запущен на порту " << serverPort << endl;

    while (isRunning) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);

        if (clientSocket >= 0) {
            thread(&HttpServer::handleClient, this, clientSocket).detach();
        }
    }
}

// Остановка сервера
void HttpServer::stop() {
    isRunning = false;
    close(serverSocket);
}

// Обработка клиентского подключения
void HttpServer::handleClient(int clientSocket) {
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }

    HttpRequest request = parseRequest(string(buffer, bytesRead));
    HttpResponse response;
    response.setCors();

    // Обработка CORS preflight запросов
    if (request.method == "OPTIONS") {
        response.statusCode = 204;
        sendResponse(clientSocket, response);
        close(clientSocket);
        return;
    }

    // Поиск подходящего маршрута
    string routeKey = request.method + " " + request.path;
    auto it = routes.find(routeKey);

    if (it != routes.end()) {
        try {
            it->second(request, response);
        } catch (const exception& e) {
            response.statusCode = 500;
            json error = {{"success", false}, {"error", e.what()}};
            response.setJson(error);
        }
    } else if (!staticFilesPath.empty() && request.method == "GET") {
        // Обслуживание статических файлов
        serveStaticFile(request.path, response);
    } else {
        response.statusCode = 404;
        json error = {{"success", false}, {"error", "Не найдено"}};
        response.setJson(error);
    }

    sendResponse(clientSocket, response);
    close(clientSocket);
}

// Парсинг HTTP запроса
HttpRequest HttpServer::parseRequest(const string& raw) {
    HttpRequest request;
    istringstream stream(raw);
    string line;

    // Парсинг строки запроса
    if (getline(stream, line)) {
        istringstream lineStream(line);
        lineStream >> request.method;

        string fullPath;
        lineStream >> fullPath;

        // Парсинг параметров запроса
        size_t queryPos = fullPath.find('?');
        if (queryPos != string::npos) {
            request.path = fullPath.substr(0, queryPos);
            string query = fullPath.substr(queryPos + 1);
            parseQueryParams(query, request.queryParams);
        } else {
            request.path = fullPath;
        }
    }

    // Парсинг заголовков
    while (getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1);
            // Удаление пробелов
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            request.headers[key] = value;
        }
    }

    // Получение длины содержимого
    int contentLength = 0;
    auto clIt = request.headers.find("Content-Length");
    if (clIt != request.headers.end()) {
        try {
            contentLength = stoi(clIt->second);
        } catch (const exception& e) {
            contentLength = 0;
        }
    }

    // Парсинг тела запроса
    if (contentLength > 0) {
        size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd != string::npos) {
            request.body = raw.substr(headerEnd + 4, contentLength);
        }
    }

    return request;
}

// Парсинг параметров запроса
void HttpServer::parseQueryParams(const string& query, map<string, string>& params) {
    istringstream stream(query);
    string pair;

    while (getline(stream, pair, '&')) {
        size_t eqPos = pair.find('=');
        if (eqPos != string::npos) {
            string key = urlDecode(pair.substr(0, eqPos));
            string value = urlDecode(pair.substr(eqPos + 1));
            params[key] = value;
        }
    }
}

// URL декодирование
string HttpServer::urlDecode(const string& encoded) {
    string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            int value;
            istringstream is(encoded.substr(i + 1, 2));
            if (is >> hex >> value) {
                decoded += static_cast<char>(value);
                i += 2;
            }
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

// Обслуживание статических файлов
void HttpServer::serveStaticFile(const string& path, HttpResponse& response) {
    string filePath = staticFilesPath;
    
    // Маппинг дружественных URL на файлы
    static const map<string, string> urlMappings = {
        {"/", "/index.html"},
        {"/login", "/pages/login.html"},
        {"/register", "/pages/register.html"},
        {"/forgot-password", "/pages/forgot-password.html"},
        {"/change-password", "/pages/change-password.html"},
        {"/events", "/pages/events.html"},
        {"/tickets", "/pages/tickets.html"},
        {"/booking-info", "/pages/booking-info.html"},
        {"/admin", "/pages/admin.html"},
        {"/ticket-detail", "/pages/ticket-detail.html"}
    };
    
    // Проверяем маппинг дружественных URL
    auto mappingIt = urlMappings.find(path);
    if (mappingIt != urlMappings.end()) {
        filePath += mappingIt->second;
    } else if (path.empty()) {
        filePath += "/index.html";
    } else {
        filePath += path;
    }

    // Защита от обхода директорий (проверяем только путь запроса, не staticFilesPath)
    if (path.find("..") != string::npos) {
        response.statusCode = 403;
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        response.body = "Доступ запрещен";
        return;
    }

    // Получение расширения файла
    string ext;
    size_t dotPos = filePath.rfind('.');
    if (dotPos != string::npos) {
        ext = filePath.substr(dotPos);
    }

    // Белый список разрешенных расширений
    static const set<string> allowedExtensions = {
        ".html", ".css", ".js", ".json", ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico", ".woff", ".woff2", ".ttf"
    };

    if (!ext.empty() && allowedExtensions.find(ext) == allowedExtensions.end()) {
        response.statusCode = 403;
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        response.body = "Запрещенный тип файла";
        return;
    }

    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        response.statusCode = 404;
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        response.body = "Файл не найден: " + path;
        return;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    response.body = buffer.str();

    // Установка Content-Type на основе расширения (с charset для текстовых файлов)
    if (ext == ".html") response.headers["Content-Type"] = "text/html; charset=utf-8";
    else if (ext == ".css") response.headers["Content-Type"] = "text/css; charset=utf-8";
    else if (ext == ".js") response.headers["Content-Type"] = "application/javascript; charset=utf-8";
    else if (ext == ".json") response.headers["Content-Type"] = "application/json; charset=utf-8";
    else if (ext == ".png") response.headers["Content-Type"] = "image/png";
    else if (ext == ".jpg" || ext == ".jpeg") response.headers["Content-Type"] = "image/jpeg";
    else if (ext == ".svg") response.headers["Content-Type"] = "image/svg+xml";
    else response.headers["Content-Type"] = "application/octet-stream";
}

// Отправка HTTP ответа
void HttpServer::sendResponse(int socket, const HttpResponse& response) {
    string statusText;
    switch (response.statusCode) {
    case 200: statusText = "OK"; break;
    case 201: statusText = "Created"; break;
    case 204: statusText = "No Content"; break;
    case 400: statusText = "Bad Request"; break;
    case 401: statusText = "Unauthorized"; break;
    case 403: statusText = "Forbidden"; break;
    case 404: statusText = "Not Found"; break;
    case 500: statusText = "Internal Server Error"; break;
    default: statusText = "Unknown"; break;
    }

    ostringstream responseStream;
    responseStream << "HTTP/1.1 " << response.statusCode << " " << statusText << "\r\n";

    for (const auto& header : response.headers) {
        responseStream << header.first << ": " << header.second << "\r\n";
    }

    responseStream << "Content-Length: " << response.body.length() << "\r\n";
    responseStream << "\r\n";
    responseStream << response.body;

    string responseStr = responseStream.str();
    send(socket, responseStr.c_str(), responseStr.length(), 0);
}
