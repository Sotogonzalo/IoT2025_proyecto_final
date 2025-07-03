set(component_srcs "main.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "tiempo_embebido" "event_logger" "wifi_embebido" "servidor_embebido" "mqtt_embebido"
                       REQUIRES "")
