add_executable(client client.cpp)

target_link_libraries(client nlohmann_json::nlohmann_json)
target_link_libraries(client Poco::Foundation)
target_link_libraries(client Poco::Net)
target_link_libraries(client Poco::Util)
target_link_libraries(client Poco::Data)
target_link_libraries(client Poco::JSON)
target_link_libraries(client Poco::Util)
target_link_libraries(client Poco::XML)
target_link_libraries(client Qt5::Core)

#install(DIRECTORY DESTINATION ./client)
if(MSVC)
install(FILES 
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll 
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll
DESTINATION ./client
)
endif(MSVC)

if(WIN32)
install(FILES 
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll 
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll
${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll
DESTINATION ./client
)
endif(WIN32)
install(DIRECTORY queries DESTINATION ./client)
install(TARGETS client CONFIGURATIONS Debug DESTINATION ./client)