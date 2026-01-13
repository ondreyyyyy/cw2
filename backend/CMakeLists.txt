# CMakeLists.txt - Конфигурация сборки проекта
# Система бронирования билетов на концерты

cmake_minimum_required(VERSION 3.10)
project(concert_booking)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск необходимых пакетов
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Поиск libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Директории заголовочных файлов
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${PQXX_INCLUDE_DIRS})

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/queryloader.cpp
    src/database.cpp
    src/password.cpp
    src/jwt.cpp
    src/auth.cpp
    src/eventservice.cpp
    src/ticketservice.cpp
    src/httpserver.cpp
    src/emailservice.cpp
    src/adminservice.cpp
)

# Заголовочные файлы
set(HEADERS
    include/config.h
    include/queryloader.h
    include/database.h
    include/password.h
    include/jwt.h
    include/auth.h
    include/eventservice.h
    include/ticketservice.h
    include/httpserver.h
    include/emailservice.h
    include/adminservice.h
    include/json.hpp
)

# Добавление исполняемого файла
add_executable(server ${SOURCES} ${HEADERS})

# Линковка библиотек
target_link_libraries(server 
    ${OPENSSL_LIBRARIES}
    ${PQXX_LIBRARIES}
    Threads::Threads
)

# Добавление путей к библиотекам
target_link_directories(server PRIVATE ${PQXX_LIBRARY_DIRS})
