set(component_srcs "src/queue_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server spiffs audio_embebido)