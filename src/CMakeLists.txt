cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(rgb_challenge)


add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(server)
