/*
 * database.cpp - Реализация класса Database
 * Работа с базой данных PostgreSQL
 */

#include "database.h"

using namespace std;

// Получение единственного экземпляра класса
Database& Database::getInstance() {
    static Database instance;
    return instance;
}

// Инициализация подключения к базе данных
void Database::initialize() {
    lock_guard<mutex> lock(dbMutex);
    
    Config& config = Config::getInstance();
    
    string connStr = "host=" + config.get("DB_HOST") +
                     " port=" + config.get("DB_PORT") +
                     " dbname=" + config.get("DB_NAME") +
                     " user=" + config.get("DB_USER") +
                     " password=" + config.get("DB_PASSWORD");
    
    connection = make_unique<pqxx::connection>(connStr);
}

// Выполнение именованного запроса с параметрами
pqxx::result Database::executeQuery(const string& queryName, 
                                     const vector<string>& params) {
    lock_guard<mutex> lock(dbMutex);
    
    string query = QueryLoader::getInstance().getQuery(queryName);
    
    pqxx::work txn(*connection);
    pqxx::result result;
    
    if (params.empty()) {
        result = txn.exec(query);
    } else {
        // Замена $1, $2 и т.д. на параметры
        pqxx::params pqParams;
        for (const auto& param : params) {
            if (param == "NULL") {
                pqParams.append();
            } else {
                pqParams.append(param);
            }
        }
        result = txn.exec_params(query, pqParams);
    }
    
    txn.commit();
    return result;
}

// Выполнение произвольного SQL запроса
pqxx::result Database::executeRawQuery(const string& query,
                                        const vector<string>& params) {
    lock_guard<mutex> lock(dbMutex);
    
    pqxx::work txn(*connection);
    pqxx::result result;
    
    if (params.empty()) {
        result = txn.exec(query);
    } else {
        pqxx::params pqParams;
        for (const auto& param : params) {
            if (param == "NULL") {
                pqParams.append();
            } else {
                pqParams.append(param);
            }
        }
        result = txn.exec_params(query, pqParams);
    }
    
    txn.commit();
    return result;
}

// Проверка состояния подключения
bool Database::isConnected() const {
    lock_guard<mutex> lock(dbMutex);
    return connection && connection->is_open();
}
