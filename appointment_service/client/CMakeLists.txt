if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/client_install" CACHE PATH "..." FORCE)
endif()

add_executable(client client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/client_install)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/client_install)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/client_install)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/client_install)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/client_install)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/client_install)

add_custom_command(TARGET client
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoDatad.dll "${CMAKE_CURRENT_BINARY_DIR}/client_install"
)
add_custom_command(TARGET client
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoFoundationd.dll "${CMAKE_CURRENT_BINARY_DIR}/client_install"
)
add_custom_command(TARGET client
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoJSONd.dll "${CMAKE_CURRENT_BINARY_DIR}/client_install"
)
add_custom_command(TARGET client
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoNetd.dll "${CMAKE_CURRENT_BINARY_DIR}/client_install"
)
add_custom_command(TARGET client
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoUtild.dll "${CMAKE_CURRENT_BINARY_DIR}/client_install"
)
add_custom_command(TARGET client
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../bin/Debug/PocoXMLd.dll "${CMAKE_CURRENT_BINARY_DIR}/client_install"
)

target_link_libraries(client nlohmann_json::nlohmann_json)
target_link_libraries(client Poco::Foundation)
target_link_libraries(client Poco::Net)
target_link_libraries(client Poco::Util)
target_link_libraries(client Poco::Data)
target_link_libraries(client Poco::JSON)
target_link_libraries(client Poco::Util)
target_link_libraries(client Poco::XML)
target_link_libraries(client Qt5::Core)

install(TARGETS client CONFIGURATIONS Debug DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/client_install")