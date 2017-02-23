cmake_minimum_required(VERSION 2.8)

project(apiserver.bin)

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -Wall -O3 ${CMAKE_CXX_FLAGS}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(COMM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common)

set(INCLUDE ${COMM_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp)
include_directories(${INCLUDE})

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp/*.cpp)

add_executable(apiserver.bin ${SRC})
target_link_libraries(apiserver.bin cppnetlib-uri cppnetlib-client-connections 
                        cppnetlib-server-parsers ssl crypto
                        thriftnb thrift boost_chrono boost_thread boost_system event rt dl
                        jsoncpp glog gflags)

