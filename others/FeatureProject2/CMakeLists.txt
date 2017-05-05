cmake_minimum_required(VERSION 2.8)

project(FeatureProject)

set(target feature.bin)

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -Wall -O3 ${CMAKE_CXX_FLAGS}")
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(INCLUDE ${SRC_DIR}/common ${SRC_DIR}/common/gen-cpp)
include_directories(${INCLUDE})

file(GLOB SRC ${SRC_DIR}/main/*.cpp ${SRC_DIR}/common/*.cpp ${SRC_DIR}/common/gen-cpp/*.cpp)

add_executable(${target} ${SRC})
target_link_libraries(${target} glog gflags jsoncpp boost_filesystem 
                    boost_system thrift thriftz z)

