

set(SOURCE_FILES util.c)

#message("SOURCE_FILES = ${SOURCE_FILES}")
add_library(util SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(util m)   # for ceil function

