# doctest single header. MIT. https://github.com/doctest/doctest
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
