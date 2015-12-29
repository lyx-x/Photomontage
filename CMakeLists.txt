cmake_minimum_required(VERSION 3.2)
project(Photomontage)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(OpenCV_STATIC OFF)
find_package(OpenCV REQUIRED)

set(SOURCE_FILES texture.cpp)
add_executable(texture ${SOURCE_FILES})

include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(texture ${OpenCV_LIBS})