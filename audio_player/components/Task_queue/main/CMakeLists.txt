


set(SOURCES "main/Main.c")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "task_queue.c"
                    PRIV_REQUIRES "driver" "color_shared"
                    REQUIRES "freertos" "esp_log"
                    )