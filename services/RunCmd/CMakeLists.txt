cmake_minimum_required (VERSION 2.8)

project(RunCmdService)            # 工程名，无特别要求

# set compiling flags   可以照抄
set(CMAKE_CXX_FLAGS "-march=native -std=c++11 -pthread -fPIC -shared -Wall -O3 ${CMAKE_CXX_FLAGS}")

# 该lib用到的其他目录
set(COMM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
set(ALG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../algorithms/RunCmd)
set(API_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../api_server)

# 指定包含头文件目录，相当于g++编译命令的 -I
set(INCLUDE ${COMM_DIR} ${API_SERVER_DIR} ${API_SERVER_DIR}/gen-cpp ${ALG_DIR}/gen-cpp)
include_directories(${INCLUDE})

# 该lib用到的源文件
# 1. 当前目录下的*.cpp
# 2. alg_mgr的由thrift生成的cpp $api_server_dir/gen-cpp
# 3. alg server端thrift生成的cpp
file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp 
        ${API_SERVER_DIR}/gen-cpp/*.cpp ${ALG_DIR}/gen-cpp/*.cpp)

add_library(runcmd SHARED ${SRC})

# 如果还需要链接其他库，在这里指定
# target_link_libraries(runcmd boost_thread boost_system)

# remove "lib" prefix for final .so target
set_target_properties(runcmd PROPERTIES PREFIX "")
