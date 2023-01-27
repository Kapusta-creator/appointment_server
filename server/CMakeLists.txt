add_executable(server start.cpp server.cpp server.h)

file(COPY create_query.sql DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
file(COPY create_clients.sql DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
file(COPY config.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/server)

if(WIN32)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll ${CMAKE_INSTALL_PREFIX}/server
)
endif(WIN32)

if(MSVC)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll ${CMAKE_INSTALL_PREFIX}/server
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll ${CMAKE_INSTALL_PREFIX}/server
)
endif(MSVC)

target_link_libraries(server nlohmann_json::nlohmann_json)
target_link_libraries(server Poco::Data)
target_link_libraries(server Poco::Foundation)
target_link_libraries(server Poco::JSON)
target_link_libraries(server Poco::Net)
target_link_libraries(server Poco::Util)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Sql)

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
install(TARGETS server CONFIGURATIONS Debug DESTINATION ${CMAKE_INSTALL_PREFIX}/server)
