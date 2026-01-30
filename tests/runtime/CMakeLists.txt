add_executable(runtime_tests
    test_runtime.cpp
    test_metal.mm
    ../../src/client/runtime.cpp
    ../../src/logging.cpp
    common.h
)

# Force gtest to use shared CRT to match the runtime tests
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

target_compile_options(runtime_tests PRIVATE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/MD>
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/MDd>
)

target_include_directories(runtime_tests PRIVATE
    ${OPENXR_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/client
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(runtime_tests PRIVATE
    OX_BUILD_STATIC
    OX_RUNTIME_TESTS
)

target_link_libraries(runtime_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
)

configure_graphics(runtime_tests)

gtest_discover_tests(runtime_tests)

# Set output directory
set_target_properties(runtime_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests/runtime
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/tests/runtime
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/tests/runtime
)