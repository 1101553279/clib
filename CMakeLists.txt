cmake_minimum_required(VERSION 3.5)

project(plc VERSION 1.0)


#exe is plc
add_executable(plc plc.c)

#header directories
include_directories(include)
include_directories(cfg)
include_directories(plog)
include_directories(util)
include_directories(cmd)
include_directories(sock)
include_directories(tick)

#cmake directories
add_subdirectory(cfg)
add_subdirectory(plog)
add_subdirectory(util)
add_subdirectory(cmd)
add_subdirectory(sock)
add_subdirectory(tick)

#link libraries
target_link_libraries(plc cfg plog util cmd sock tick)

