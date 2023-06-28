

set(SOURCE_FILES plog.c log.c udp/udp.c udp/udp_driver.c udp/udp_device.c udp/argudp.c)

#message("SOURCE_FILES = ${SOURCE_FILES}")
add_library(log ${SOURCE_FILES})

target_link_libraries(log cmd)


