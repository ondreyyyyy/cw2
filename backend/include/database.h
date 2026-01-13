/*
 * database.h - Заголовочный файл для работы с базой данных
 * Содержит класс Database для подключения и выполнения запросов к PostgreSQL
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include "config.h"
#include "queryloader.h"

using namespace std;

// Класс для работы с базой данных PostgreSQL
class Database {
public:
    // Получение единственного экземпляра класса (синглтон)
    static Database& getInstance();

    // Инициализация подключения к базе данных
    void initialize();

    // Выполнение именованного запроса с параметрами
    pqxx::result executeQuery(const string& queryName, 
                              const vector<string>& params = {});

    // Выполнение произвольного SQL запроса
    pqxx::result executeRawQuery(const string& query,
                                  const vector<string>& params = {});

    // Проверка состояния подключения
    bool isConnected() const;

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    unique_ptr<pqxx::connection> connection;  // Подключение к БД
    mutable mutex dbMutex;                     // Мьютекс для потокобезопасности
};

#endif // DATABASE_H
