cmake_minimum_required (VERSION 2.8)

project(topics_update)

# set compiling flags
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-march=native -std=c++11 -pthread -fPIC -shared -O3 ${CMAKE_CXX_FLAGS}")

set(COMM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
set(ALG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../algorithms/topics_update/test)
set(API_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../api_server)
set(INCLUDE ${COMM_DIR} ${API_SERVER_DIR} ${API_SERVER_DIR}/gen-cpp ${ALG_DIR}/gen-cpp)

include_directories(${INCLUDE})

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp 
        ${API_SERVER_DIR}/gen-cpp/*.cpp ${ALG_DIR}/gen-cpp/*.cpp)

add_library(topic_service_update SHARED ${SRC})

# remove "lib" prefix for final .so target
set_target_properties(topic_service_update PROPERTIES PREFIX "")
