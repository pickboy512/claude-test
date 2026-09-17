cmake_minimum_required(VERSION 3.10)
project(Snake3D CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(snake3d src/main.cpp)
target_link_libraries(snake3d PRIVATE raylib)

if(APPLE)
    target_link_libraries(snake3d PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()
