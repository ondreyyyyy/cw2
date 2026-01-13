/*
 * config.cpp - Реализация класса Config
 * Загрузка и управление конфигурацией приложения
 */

#include "config.h"

using namespace std;

// Получение единственного экземпляра класса
Config& Config::getInstance() {
    static Config instance;
    return instance;
}

// Загрузка конфигурации из файла
void Config::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл конфигурации: " + filename);
    }

    string line;
    while (getline(file, line)) {
        // Пропуск пустых строк и комментариев
        if (line.empty() || line[0] == '#') continue;

        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            
            // Удаление пробелов
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            configData[key] = value;
        }
    }
    file.close();
}

// Получение значения по ключу
string Config::get(const string& key) const {
    auto it = configData.find(key);
    if (it == configData.end()) {
        throw runtime_error("Ключ конфигурации не найден: " + key);
    }
    return it->second;
}

// Получение значения по ключу с значением по умолчанию
string Config::get(const string& key, const string& defaultValue) const {
    auto it = configData.find(key);
    if (it == configData.end()) {
        return defaultValue;
    }
    return it->second;
}

// Получение целочисленного значения по ключу
int Config::getInt(const string& key) const {
    return stoi(get(key));
}

// Получение целочисленного значения по ключу с значением по умолчанию
int Config::getInt(const string& key, int defaultValue) const {
    try {
        return stoi(get(key));
    } catch (const exception& e) {
        return defaultValue;
    }
}
