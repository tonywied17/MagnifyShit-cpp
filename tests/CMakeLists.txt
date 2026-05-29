add_executable(magshit_tests
    test_main.cpp
    test_geometry.cpp
    test_result.cpp
)

target_link_libraries(magshit_tests PRIVATE doctest)
target_include_directories(magshit_tests PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(magshit_tests PRIVATE /W4 /WX)
endif()

include(CTest)
add_test(NAME magshit_tests COMMAND magshit_tests)
