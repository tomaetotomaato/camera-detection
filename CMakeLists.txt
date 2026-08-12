cmake_minimum_required(VERSION 3.14)
project(camera_detection CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release: an unoptimised build makes YOLO inference several times slower.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

find_package(OpenCV REQUIRED)

set(ONNXRUNTIME_ROOT_DIR "$ENV{HOME}/onnxruntime")
set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_ROOT_DIR}/include")
set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_ROOT_DIR}/lib")

set(YOLOS_CPP_INCLUDE_DIR "$ENV{HOME}/YOLOs-CPP/include")

# Fail at configure time with a readable message rather than at link time.
if(NOT EXISTS "${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so")
    message(FATAL_ERROR "ONNX Runtime not found at ${ONNXRUNTIME_LIB_DIR}")
endif()
if(NOT EXISTS "${YOLOS_CPP_INCLUDE_DIR}/yolos/tasks/detection.hpp")
    message(FATAL_ERROR "YOLOs-CPP headers not found at ${YOLOS_CPP_INCLUDE_DIR}")
endif()

add_executable(camera_detection
    src/main.cpp
    src/TestCameraSource.cpp
    src/VideoCameraSource.cpp
    src/YoloDetector.cpp
    src/Visualization.cpp
)

target_include_directories(camera_detection PRIVATE
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${YOLOS_CPP_INCLUDE_DIR}
)

target_link_libraries(camera_detection PRIVATE
    ${OpenCV_LIBS}
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so
)
