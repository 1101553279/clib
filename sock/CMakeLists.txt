

set(SOURCE_FILES sock.c tcps.c udps.c sutil.c sclient.c)

#message("SOURCE_FILES = ${SOURCE_FILES}")
add_library(sock ${SOURCE_FILES})

#target_link_libraries(sock)


