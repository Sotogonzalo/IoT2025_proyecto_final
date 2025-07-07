set(component_srcs "src/mqtt_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES mqtt json queue_embebido event_logger)
