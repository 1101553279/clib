

set(SOURCE_FILES command.c cbasic.c argtable3.c)


#message("SOURCE_FILES = ${SOURCE_FILES}")
#add_library(cmd SHARED ${SOURCE_FILES})
add_library(cmd ${SOURCE_FILES})


target_link_libraries(cmd pthread util log)

