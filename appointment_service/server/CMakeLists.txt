if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/server_install" CACHE PATH "..." FORCE)
endif()

add_executable(server start.cpp server.cpp server.h)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll DESTINATION .)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll DESTINATION .)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll DESTINATION .)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll DESTINATION .)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll DESTINATION .)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll DESTINATION .)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll "${CMAKE_CURRENT_BINARY_DIR}/server_install"
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll "${CMAKE_CURRENT_BINARY_DIR}/server_install"
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll "${CMAKE_CURRENT_BINARY_DIR}/server_install"
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll "${CMAKE_CURRENT_BINARY_DIR}/server_install"
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll "${CMAKE_CURRENT_BINARY_DIR}/server_install"
)
add_custom_command(TARGET server
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll "${CMAKE_CURRENT_BINARY_DIR}/server_install"
)

target_link_libraries(server nlohmann_json::nlohmann_json)
target_link_libraries(server Poco::Data)
target_link_libraries(server Poco::Foundation)
target_link_libraries(server Poco::JSON)
target_link_libraries(server Poco::Net)
target_link_libraries(server Poco::Util)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Sql)

install(TARGETS server CONFIGURATIONS Debug DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/server_install")
file(COPY create_query.sql DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/server_install")
file(COPY create_clients.sql DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/server_install")
#install(TARGETS server DESTINATION .)