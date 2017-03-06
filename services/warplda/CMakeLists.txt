cmake_minimum_required (VERSION 2.8)

project (topics)

# add_subdirectory(gflags)

# set compiling flags
set(CMAKE_CXX_FLAGS "-march=native -std=c++11 -pthread -fPIC -shared -O3 ${CMAKE_CXX_FLAGS}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../api_server/ServiceLibs)

set(COMM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
set(ALG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../algorithms/warplda/src)
set(API_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../api_server)
set(INCLUDE ${COMM_DIR} ${API_SERVER_DIR} ${API_SERVER_DIR}/gen-cpp ${ALG_DIR}/gen-cpp)

include_directories(${INCLUDE})

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp 
        ${API_SERVER_DIR}/gen-cpp/*.cpp ${ALG_DIR}/gen-cpp/*.cpp)

add_library(topic_service SHARED ${SRC})

# remove "lib" prefix for final .so target
set_target_properties(topic_service PROPERTIES PREFIX "")
