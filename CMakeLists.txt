cmake_minimum_required(VERSION 3.10)
project(NetworkServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)

# Исходные файлы сервера
set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/server_process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server_display.cpp
    ${COMMON_DIR}/socket.cpp
    ${COMMON_DIR}/queue.cpp
    ${COMMON_DIR}/http.cpp
    ${COMMON_DIR}/http.cpp
)

# Исходные файлы клиента
set(CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${COMMON_DIR}/socket.cpp
    ${COMMON_DIR}/http.cpp
)

# Сервер обработки данных
add_executable(server_process 
    ${CMAKE_CURRENT_SOURCE_DIR}/server_process.cpp
    ${COMMON_DIR}/socket.cpp
    ${COMMON_DIR}/queue.cpp
    ${COMMON_DIR}/http.cpp
)
target_include_directories(server_process PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMMON_DIR}
)

# Сервер отображения
add_executable(server_display 
    ${CMAKE_CURRENT_SOURCE_DIR}/server_display.cpp
    ${COMMON_DIR}/socket.cpp
    ${COMMON_DIR}/http.cpp
)
target_include_directories(server_display PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMMON_DIR}
)

# Клиент
add_executable(client 
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${COMMON_DIR}/socket.cpp
    ${COMMON_DIR}/http.cpp
)
target_include_directories(client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)
