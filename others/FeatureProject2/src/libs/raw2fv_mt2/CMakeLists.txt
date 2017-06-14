set(target raw2fv_mt2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -shared")
# message("raw2fv CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${COMM_SRC}
        ${UTILS_DIR}/read_cmd.cpp)
# message("raw2fv SRC = ${SRC}")

add_library(${target} SHARED ${SRC})
target_link_libraries(${target} ${COMM_LIBS} boost_iostreams boost_thread)
set_target_properties(${target} PROPERTIES PREFIX "")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # message("This is Apple OS.")
    set_target_properties(${target} PROPERTIES SUFFIX ".so")
endif()
