

set(SOURCE_FILES console.c command.c cmdlist.c usrv.c)


#message("SOURCE_FILES = ${SOURCE_FILES}")
add_library(cmd SHARED ${SOURCE_FILES})


target_link_libraries(cmd plog pthread util)

