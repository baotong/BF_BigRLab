set(target feature.bin)

# message(STATUS "COMM_DIR=${COMM_DIR}")
# message(STATUS "INCLUDE=${INCLUDE}")
# message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
# message(STATUS "PROJECT_DIR=${PROJECT_DIR}")

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${COMM_SRC})
# message(STATUS "SRC=${SRC}")

add_executable(${target} ${SRC})
target_link_libraries(${target} ${COMM_LIBS} gflags dl)

