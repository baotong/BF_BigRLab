set(target article2vec)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -shared")

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${UTILS_DIR}/read_cmd.cpp)

add_library(${target} SHARED ${SRC})
target_link_libraries(${target} glog jsoncpp boost_iostreams)
set_target_properties(${target} PROPERTIES PREFIX "")

