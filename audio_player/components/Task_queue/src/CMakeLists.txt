

set(SRCS "src/Task_queue.c")
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" "color_shared"
                       REQUIRES "")


                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" "color_shared"
                       REQUIRES "")
