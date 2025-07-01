set(component_srcs "src/config_embebido.c")

idf_component_register(
    SRCS "${component_srcs}"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
)
