cmake_minimum_required(VERSION 3.5)
project(assignment2)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES a2_fall2017.c)

add_executable(a2_fall2017 ${SOURCE_FILES})

# Add -lrt flag to compiler for shm_open to work.
target_link_libraries(a2_fall2017 rt)
target_link_libraries(a2_fall2017 pthread)

