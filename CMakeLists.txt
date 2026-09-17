cmake_minimum_required(VERSION 3.10)
project(YasuoPracticeTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(practice_tool src/main.cpp)
target_link_libraries(practice_tool PRIVATE raylib)

if(APPLE)
    target_link_libraries(practice_tool PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()
