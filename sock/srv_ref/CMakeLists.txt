cmake_minimum_required(VERSION 3.5)

project(srv VERSION 2.0)


add_executable(srv main.c srv.c client.c sig.c)

#target_link_libraries(simple ${GLIB2_LIBRARIES})
