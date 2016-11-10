SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

SET(COMM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../common")
SET(API_SERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../api_server")
SET(INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp" "${COMM_DIR}" "${API_SERVER_DIR}/gen-cpp")

# MESSAGE( STATUS "COMM_DIR: " ${COMM_DIR} )
# MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
# MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR} )

add_definitions(-DNDEBUG)

INCLUDE_DIRECTORIES(${INCLUDE})

# include_directories(${INCLUDE})
file(GLOB SRC demo.cpp Article2Vector.cpp ArticleServiceHandler.cpp 
    ClusterPredict.cpp Article2Vector.cpp WordClusterDB.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp/*.cpp ${API_SERVER_DIR}/gen-cpp/*.cpp)

SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fopenmp -O3 -g")

ADD_EXECUTABLE(demo ${SRC})
target_link_libraries(demo thriftnb thrift boost_chrono boost_thread boost_system event jsoncpp glog gflags)
# ADD_EXECUTABLE(load_test load_test.cpp)
# ADD_SUBDIRECTORY(unittest)
