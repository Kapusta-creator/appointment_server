cmake_minimum_required(VERSION 3.13)

project(client_cmake)

add_executable(client client.cpp)

include(FetchContent)

find_package(Qt5 COMPONENTS Core REQUIRED)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.11.2
)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(poco
    GIT_REPOSITORY https://github.com/pocoproject/poco
    GIT_TAG 3e14d408525a6325b16aa18154a6603da06cd769
)

FetchContent_GetProperties(poco)
if(NOT poco_POPULATED)
    FetchContent_Populate(poco)
    add_subdirectory(${poco_SOURCE_DIR} ${poco_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


target_link_libraries(client nlohmann_json::nlohmann_json)
target_link_libraries(client Poco::Foundation)
target_link_libraries(client Poco::Net)
target_link_libraries(client Poco::Util)
target_link_libraries(client Poco::Data)
target_link_libraries(client Poco::JSON)
target_link_libraries(client Qt5::Core)

install(TARGETS client CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install)
