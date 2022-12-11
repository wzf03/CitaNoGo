cmake_minimum_required(VERSION 3.10)

project(Cita)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BOT_SIMPLE_REACT "use simple react" ON)

find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(${PROJECT_NAME} main.cc board.cc mcts.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

if(NOT BOT_SIMPLE_REACT)
    add_definitions(-D_BOTZONE_ONLINE)
endif()