/*
 * queryloader.h - Заголовочный файл для загрузчика SQL запросов
 * Содержит класс QueryLoader для загрузки SQL запросов из файла
 */

#ifndef QUERYLOADER_H
#define QUERYLOADER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <mutex>

using namespace std;

// Класс для загрузки и хранения SQL запросов
class QueryLoader {
public:
    // Получение единственного экземпляра класса (синглтон)
    static QueryLoader& getInstance();

    // Загрузка запросов из файла
    void loadFromFile(const string& filename);

    // Получение запроса по имени
    string getQuery(const string& queryName) const;

    // Проверка наличия запроса
    bool hasQuery(const string& queryName) const;

private:
    QueryLoader() = default;
    QueryLoader(const QueryLoader&) = delete;
    QueryLoader& operator=(const QueryLoader&) = delete;

    map<string, string> queries;  // Хранилище запросов
    mutable mutex queryMutex;     // Мьютекс для потокобезопасности
};

#endif // QUERYLOADER_H
