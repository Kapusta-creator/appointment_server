cmake_minimum_required(VERSION 3.13)

project(server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "..." FORCE)
endif()

include(CPM.cmake)

set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/_cpm.cache)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)

add_executable(server start.cpp server.cpp server.h)


find_package(Qt5 COMPONENTS Sql Core REQUIRED)
include(FetchContent)


FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.11.2
)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

CPMAddPackage("gh:pocoproject/poco#poco-1.12.4-release")

target_link_libraries(server nlohmann_json::nlohmann_json)
target_link_libraries(server Poco::Data)
target_link_libraries(server Poco::DataSQLite)
target_link_libraries(server Poco::Foundation)
target_link_libraries(server Poco::JSON)
target_link_libraries(server Poco::Net)
target_link_libraries(server Poco::Util)
target_link_libraries(server Poco::XML)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Sql)

set_target_properties(server PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS server CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install)
#install(TARGETS server DESTINATION .)
