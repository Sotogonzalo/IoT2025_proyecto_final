set(component_srcs "src/event_logger.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES nvs_flash tiempo_embebido)
