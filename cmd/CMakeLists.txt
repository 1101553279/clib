

set(SOURCE_FILES command.c usrv.c)


#message("SOURCE_FILES = ${SOURCE_FILES}")
add_library(cmd SHARED ${SOURCE_FILES})


target_link_libraries(cmd pthread plog util)

