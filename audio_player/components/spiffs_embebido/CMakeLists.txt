set(SOURCES "src/spiffs_embebido.c")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES spiffs)

