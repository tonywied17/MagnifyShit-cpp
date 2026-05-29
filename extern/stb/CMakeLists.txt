# Header-only single-file libs (stb_image, stb_image_write).
# Public domain. https://github.com/nothings/stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
