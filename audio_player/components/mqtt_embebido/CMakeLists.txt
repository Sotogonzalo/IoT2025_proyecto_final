set(component_srcs "src/mqtt_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "audio_embebido"
                       REQUIRES "mqtt" "json")
