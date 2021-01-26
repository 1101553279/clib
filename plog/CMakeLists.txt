

set(SOURCE_FILES plog.c)

#message("SOURCE_FILES = ${SOURCE_FILES}")
add_library(plog SHARED ${SOURCE_FILES})

target_link_libraries(plog cmd)


