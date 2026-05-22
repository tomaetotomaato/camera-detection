cmake_minimum_required(VERSION 3.14)
project(camera_detection CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

add_executable(camera_detection src/main.cpp src/TestCameraSource.cpp)

target_link_libraries(camera_detection PRIVATE ${OpenCV_LIBS})