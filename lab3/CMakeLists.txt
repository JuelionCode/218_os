cmake_minimum_required(VERSION 3.16)
project(lab3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -O2 -g)

# В Linux для shm_open требуется librt, для семафоров - pthread
find_package(Threads REQUIRED)

add_executable(parent src/parent.c)
target_link_libraries(parent PRIVATE Threads::Threads rt)

add_executable(child src/child.c)
target_link_libraries(child PRIVATE Threads::Threads rt)
