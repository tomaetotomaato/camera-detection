cmake_minimum_required(VERSION 3.14)
project(camera_detection CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

add_executable(camera_detection src/main.cpp)

target_link_libraries(camera_detection PRIVATE ${OpenCV_LIBS})