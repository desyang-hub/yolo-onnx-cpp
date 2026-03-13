cmake_minimum_required(VERSION 3.18)
project(yolo_onnx_cpu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCV REQUIRED)

# 支持从命令行传入 ONNXRUNTIME_ROOT
if(NOT ONNXRUNTIME_ROOT)
    message(FATAL_ERROR "Please set ONNXRUNTIME_ROOT via -DONNXRUNTIME_ROOT=...")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)


include_directories(${ONNXRUNTIME_ROOT}/include)
link_directories(${ONNXRUNTIME_ROOT}/lib)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(yolo_onnx_cpu ${SOURCES})
target_link_libraries(yolo_onnx_cpu
    ${OpenCV_LIBS}
    onnxruntime
    pthread
)