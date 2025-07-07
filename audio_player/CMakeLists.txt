# The following four lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(audio_player)

spiffs_create_partition_image(spiffs spiffs "${CMAKE_SOURCE_DIR}/spiffs")
idf_build_set_property(__FLASH_DEPENDS "spiffs-flash" APPEND)