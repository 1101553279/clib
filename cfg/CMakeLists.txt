

set(SOURCE_FILES cfg.c inih/ini.c)


#message("SOURCE_FILES = ${SOURCE_FILES}")
#add_library(cmd SHARED ${SOURCE_FILES})
add_library(cfg ${SOURCE_FILES})
