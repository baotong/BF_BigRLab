set(target raw2fv)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -shared")
# message("raw2fv CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${COMM_SRC}
        ${UTILS_DIR}/read_cmd.cpp)
# message("raw2fv SRC = ${SRC}")

add_library(${target} SHARED ${SRC})
target_link_libraries(${target} ${COMM_LIBS} boost_iostreams)
set_target_properties(${target} PROPERTIES PREFIX "")

