cmake_minimum_required(VERSION 2.8)

project(koda_compression)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} include)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

set(SRCS
  src/main.cpp
  src/movie.cpp
  src/psnr.cpp
  src/compression.cpp
)

add_executable(koda_compression ${SRCS})

target_link_libraries(koda_compression ${OpenCV_LIBS})