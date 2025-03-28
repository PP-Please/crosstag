cmake_minimum_required(VERSION 4.0)
project(CrossTag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

find_package(SQLite3 REQUIRED)
add_executable(crosstag src/main.cpp src/winFunctions.cpp)

target_link_libraries(crosstag PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(crosstag PRIVATE SQLite::SQLite3)