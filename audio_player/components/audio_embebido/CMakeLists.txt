set(srcs "src/audio_embebido.c")
set(include_dirs "include")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES es8311 driver config_embebido event_logger)
