SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

SET(COMM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../common")
SET(API_SERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../api_server")
SET(INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp" "${COMM_DIR}" "${API_SERVER_DIR}/gen-cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kenlm")

# MESSAGE( STATUS "COMM_DIR: " ${COMM_DIR} )
# MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
# MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR} )

INCLUDE_DIRECTORIES(${INCLUDE})

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/kenlm/build/lib")

add_definitions(-DKENLM_MAX_ORDER=6)
add_definitions(-DHAVE_ZLIB)

# include_directories(${INCLUDE})
file(GLOB SRC demo.cpp Article2Vector.cpp ArticleServiceHandler.cpp ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp/*.cpp ${API_SERVER_DIR}/gen-cpp/*.cpp)

SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fopenmp -O3 -g")

ADD_EXECUTABLE(demo ${SRC})
target_link_libraries(demo thriftnb thrift boost_chrono boost_thread boost_system event jsoncpp glog gflags kenlm kenlm_util z bz2)
# ADD_EXECUTABLE(load_test load_test.cpp)
# ADD_SUBDIRECTORY(unittest)
