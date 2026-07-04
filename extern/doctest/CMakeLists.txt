# doctest single header. MIT. https://github.com/doctest/doctest
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Use real std headers instead of doctest's forward-declaration shims for std::tuple etc.
# Newer MSVC (C5285) rejects those forward declarations, which breaks the /WX test build.
target_compile_definitions(doctest INTERFACE DOCTEST_CONFIG_USE_STD_HEADERS)
