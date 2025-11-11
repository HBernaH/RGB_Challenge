add_executable(server_exec
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)
target_link_libraries(server_exec PRIVATE datagram file_tools)
target_include_directories(server_exec PRIVATE ${CMAKE_SOURCE_DIR}/lib)