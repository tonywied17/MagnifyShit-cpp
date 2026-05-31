cmake_minimum_required(VERSION 3.24)
project(MagnifyShit
    VERSION 2.1.0
    DESCRIPTION "Modern GPU-accelerated desktop magnifier"
    HOMEPAGE_URL "https://github.com/tonywied17/MagnifyShit-cpp"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Static MSVC runtime is intentionally NOT used so the exe stays small and
# matches what users already have via VC++ redistributables.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# LTO / IPO for optimized configs only.
include(CheckIPOSupported)
check_ipo_supported(RESULT MAGSHIT_IPO_OK OUTPUT MAGSHIT_IPO_ERR)
if(MAGSHIT_IPO_OK)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL     ON)
else()
    message(STATUS "IPO/LTO not supported: ${MAGSHIT_IPO_ERR}")
endif()

if(MSVC)
    add_compile_options(
        /W4 /permissive- /Zc:__cplusplus /Zc:inline /Zc:preprocessor /utf-8 /MP
    )
    # Optimized configs: enable function/data-level linking so the linker can
    # drop unreferenced symbols and fold identical COMDATs.
    add_compile_options(
        $<$<NOT:$<CONFIG:Debug>>:/Gy>
        $<$<NOT:$<CONFIG:Debug>>:/Gw>
        $<$<NOT:$<CONFIG:Debug>>:/Oi>
    )
    add_link_options(
        $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
        $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
        $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
        $<$<NOT:$<CONFIG:Debug>>:/DEBUG:NONE>
    )
    # Debug: enable expand-once inlining so size doesn't balloon as badly.
    add_compile_options($<$<CONFIG:Debug>:/Ob1>)

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

# ---- Install + packaging --------------------------------------------------
install(TARGETS MagnifyShit RUNTIME DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/README.md"
              "${CMAKE_SOURCE_DIR}/LICENSE"
        DESTINATION .)

set(CPACK_PACKAGE_NAME              "MagnifyShit")
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR            "tonywied17")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME         "MagnifyShit-${PROJECT_VERSION}-win64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR                 "ZIP")
set(CPACK_STRIP_FILES               TRUE)
include(CPack)
