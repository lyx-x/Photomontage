cmake_minimum_required(VERSION 3.2)
project(Photomontage)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(OpenCV_STATIC OFF)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(texture texture.cpp)
target_link_libraries(texture ${OpenCV_LIBS})

add_executable(montage photomontage.cpp Graph.cpp Graph.h)
target_link_libraries(montage ${OpenCV_LIBS})