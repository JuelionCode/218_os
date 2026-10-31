cmake_minimum_required(VERSION 3.10)
project(os_lab2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

find_package(Threads REQUIRED)

add_executable(kmeans
    main.c
    kmeans_serial.c
    kmeans_parallel.c
    kmeans.h
)

target_link_libraries(kmeans PRIVATE Threads::Threads m)
