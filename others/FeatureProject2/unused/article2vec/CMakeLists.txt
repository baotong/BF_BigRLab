set(target article2vec.bin)

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${UTILS_DIR}/read_cmd.cpp)

add_executable(${target} ${SRC})
target_link_libraries(${target} glog gflags boost_iostreams)

