cmake_minimum_required(VERSION 3.5)

project(clib VERSION 1.0)


#exe is run
add_executable(run main.c init/init.c)

#header directories
include_directories(./)
#include_directories(cfg)
#include_directories(log)
#include_directories(util)
#include_directories(cmd)
#include_directories(sock)
#include_directories(tick)

#cmake directories
add_subdirectory(cfg)
add_subdirectory(log)
add_subdirectory(util)
add_subdirectory(cmd)
add_subdirectory(sock)
add_subdirectory(tick)

#link libraries
target_link_libraries(run log util cmd sock tick cfg)

