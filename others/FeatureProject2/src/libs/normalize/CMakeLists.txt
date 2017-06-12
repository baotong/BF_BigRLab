set(target normalize)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -shared")

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${COMM_SRC})

add_library(${target} SHARED ${SRC})
target_link_libraries(${target} ${COMM_LIBS})
set_target_properties(${target} PROPERTIES PREFIX "")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # message("This is Apple OS.")
    set_target_properties(${target} PROPERTIES SUFFIX ".so")
endif()
