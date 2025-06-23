set(component_srcs "src/servidor_embebido.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server)

target_add_binary_data(${COMPONENT_LIB} "html/index.html" TEXT)
