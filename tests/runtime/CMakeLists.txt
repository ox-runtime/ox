set(RUNTIME_TEST_SOURCES
    test_runtime.cpp
    ../../src/client/runtime.cpp
    ../../src/logging.cpp
    common.h
)

# Add Metal test for macOS
if(APPLE AND OX_METAL)
    list(APPEND RUNTIME_TEST_SOURCES test_metal.mm)
endif()

add_executable(runtime_tests ${RUNTIME_TEST_SOURCES})

# Add Metal implementation for macOS
if(APPLE AND OX_METAL)
    target_sources(runtime_tests PRIVATE ../../src/client/metal_swapchain.mm)
    set_source_files_properties(../../src/client/metal_swapchain.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

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