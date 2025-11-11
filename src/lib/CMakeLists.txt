add_library(file_tools 
    ${CMAKE_CURRENT_SOURCE_DIR}/file_tools.cpp)
target_include_directories(file_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(datagram 
    ${CMAKE_CURRENT_SOURCE_DIR}/datagram.cpp)

target_include_directories(datagram PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(datagram PUBLIC file_tools)