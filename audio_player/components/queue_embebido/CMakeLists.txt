set(component_srcs "src/queue_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "audio_embebido;queue_embebido"
                       REQUIRES esp_http_server spiffs)