set(component_srcs "src/sntp.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "wifi_embebido"
                       REQUIRES "")