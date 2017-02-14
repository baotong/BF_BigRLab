cmake_minimum_required (VERSION 2.8)

project(RunCmdSvr)

set(CMAKE_CXX_FLAGS "-march=native -std=c++11 -pthread -Wall -O3 ${CMAKE_CXX_FLAGS}")

set(COMM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../common")
set(ALG_COMM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../alg_common")
set(API_SERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../api_server")
set(INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp" "${COMM_DIR}" "${ALG_COMM_DIR}" "${API_SERVER_DIR}/gen-cpp")

include_directories(${INCLUDE})

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp/*.cpp ${API_SERVER_DIR}/gen-cpp/*.cpp)

add_executable(RunCmdSvr.bin ${SRC})
target_link_libraries(RunCmdSvr.bin thriftnb thrift boost_chrono boost_thread boost_system 
                        boost_iostreams event jsoncpp glog gflags)

