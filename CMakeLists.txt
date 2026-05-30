cmake_minimum_required(VERSION 3.24)
project(MagnifyShit
    VERSION 2.0.1
    DESCRIPTION "Modern GPU-accelerated desktop magnifier"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /MP)
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

add_subdirectory(extern)
add_subdirectory(src)

option(MAGSHIT_BUILD_TESTS "Build unit tests" ON)
if(MAGSHIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
