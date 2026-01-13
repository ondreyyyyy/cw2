/*
 * queryloader.cpp - Реализация класса QueryLoader
 * Загрузка SQL запросов из файла
 */

#include "queryloader.h"

using namespace std;

// Получение единственного экземпляра класса
QueryLoader& QueryLoader::getInstance() {
    static QueryLoader instance;
    return instance;
}

// Загрузка запросов из файла
void QueryLoader::loadFromFile(const string& filename) {
    lock_guard<mutex> lock(queryMutex);
    
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл запросов: " + filename);
    }

    string line;
    string currentQueryName;
    stringstream currentQuery;

    while (getline(file, line)) {
        // Проверка, является ли строка именем запроса
        if (line.find("-- @") == 0) {
            // Сохранение предыдущего запроса, если есть
            if (!currentQueryName.empty()) {
                queries[currentQueryName] = currentQuery.str();
                currentQuery.str("");
                currentQuery.clear();
            }
            // Извлечение имени запроса
            currentQueryName = line.substr(4);
            // Удаление пробелов
            currentQueryName.erase(0, currentQueryName.find_first_not_of(" \t"));
            currentQueryName.erase(currentQueryName.find_last_not_of(" \t\r\n") + 1);
        } else if (!currentQueryName.empty()) {
            // Пропуск пустых строк в начале запроса
            if (currentQuery.str().empty() && line.empty()) continue;
            // Пропуск строк комментариев
            if (line.find("--") == 0 && line.find("-- @") != 0) continue;
            
            if (!currentQuery.str().empty()) {
                currentQuery << " ";
            }
            currentQuery << line;
        }
    }

    // Сохранение последнего запроса
    if (!currentQueryName.empty()) {
        queries[currentQueryName] = currentQuery.str();
    }

    file.close();
}

// Получение запроса по имени
string QueryLoader::getQuery(const string& queryName) const {
    lock_guard<mutex> lock(queryMutex);
    
    auto it = queries.find(queryName);
    if (it == queries.end()) {
        throw runtime_error("Запрос не найден: " + queryName);
    }
    return it->second;
}

// Проверка наличия запроса
bool QueryLoader::hasQuery(const string& queryName) const {
    lock_guard<mutex> lock(queryMutex);
    return queries.find(queryName) != queries.end();
}
