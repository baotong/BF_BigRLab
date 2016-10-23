SET(COMM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../common")
SET(ALG_COMM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../alg_common")
SET(API_SERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../api_server")
SET(INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp" "${COMM_DIR}" "${ALG_COMM_DIR}" "${API_SERVER_DIR}/gen-cpp")

INCLUDE_DIRECTORIES(${INCLUDE})

add_library (common Bigraph.cpp AdjList.cpp Vocab.cpp NumaArray.cpp clock.cpp)
set (LINK_LIBS ${LINK_LIBS} common numa thriftnb thrift boost_chrono boost_thread boost_system event jsoncpp glog gflags)

file(GLOB SRC topics_svr.cpp lda.cpp warplda.cpp TopicModule.cpp TopicServiceHandler.cpp ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp/*.cpp ${API_SERVER_DIR}/gen-cpp/*.cpp)

# add_executable (warplda main.cpp lda.cpp warplda.cpp)
# add_executable (format format.cpp)
# add_executable (demo demo.cpp lda.cpp warplda.cpp)
add_executable(topics_svr ${SRC})

target_link_libraries (topics_svr ${LINK_LIBS})
# target_link_libraries (warplda ${LINK_LIBS})
# target_link_libraries (format ${LINK_LIBS})
# target_link_libraries (demo ${LINK_LIBS})
