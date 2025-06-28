set(component_srcs "main.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       EMBED_FILES "canon.pcm"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "wifi_embebido" "servidor_embebido" "mqtt_embebido" "audio_embebido"
                       REQUIRES "")
