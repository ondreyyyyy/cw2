/*
 * config.h - Заголовочный файл для работы с конфигурацией
 * Содержит класс Config для загрузки переменных окружения из файла
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

// Класс для работы с конфигурацией приложения
class Config {
public:
    // Получение единственного экземпляра класса (синглтон)
    static Config& getInstance();

    // Загрузка конфигурации из файла
    void loadFromFile(const string& filename);

    // Получение значения по ключу
    string get(const string& key) const;

    // Получение значения по ключу с значением по умолчанию
    string get(const string& key, const string& defaultValue) const;

    // Получение целочисленного значения по ключу
    int getInt(const string& key) const;

    // Получение целочисленного значения по ключу с значением по умолчанию
    int getInt(const string& key, int defaultValue) const;

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    map<string, string> configData;  // Хранилище конфигурации
};

#endif // CONFIG_H
