cmake_minimum_required(VERSION 3.14)
project(camera_detection CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

set(ONNXRUNTIME_ROOT_DIR "$ENV{HOME}/onnxruntime")
set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_ROOT_DIR}/include")
set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_ROOT_DIR}/lib")

set(YOLOS_CPP_INCLUDE_DIR "$ENV{HOME}/YOLOs-CPP/include")

add_executable(camera_detection 
    src/main.cpp 
    src/TestCameraSource.cpp
)

target_include_directories(camera_detection PRIVATE
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${YOLOS_CPP_INCLUDE_DIR}
)

target_link_libraries(camera_detection PRIVATE 
    ${OpenCV_LIBS} 
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so
)