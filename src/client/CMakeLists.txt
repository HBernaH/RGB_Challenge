add_executable(client_exec
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
)
target_link_libraries(client_exec PRIVATE datagram file_tools)
target_include_directories(client_exec PRIVATE ${CMAKE_SOURCE_DIR}/lib)