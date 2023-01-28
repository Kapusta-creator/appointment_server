add_executable(server start.cpp server.cpp server.h)

target_link_libraries(server nlohmann_json::nlohmann_json)
target_link_libraries(server Poco::Data)
target_link_libraries(server Poco::Foundation)
target_link_libraries(server Poco::JSON)
target_link_libraries(server Poco::Net)
target_link_libraries(server Poco::Util)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Sql)

#install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
install(FILES create_query.sql create_clients.sql config.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
if(MSVC)
install(FILES 
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll
DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
endif(MSVC)
if(WIN32)
install(FILES 
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll
DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
endif(WIN32)
install(TARGETS server CONFIGURATIONS Debug DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
