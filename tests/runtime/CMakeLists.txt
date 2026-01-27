cmake_minimum_required(VERSION 3.15)

# Runtime tests executable
add_executable(runtime_tests
    runtime_tests.cpp
    mock_service_connection.h
    mock_service_connection.cpp
    ../../src/logging.cpp  # Include logging implementation
)

target_include_directories(runtime_tests PRIVATE
    ${OPENXR_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link against GTest, OpenXR loader, and ox runtime
# Tests use the loader to call runtime functions, with mock service injected
target_link_libraries(runtime_tests PRIVATE
    GTest::gtest_main
    openxr_loader
    ox
)

# Use /MD for this target to match openxr_loader
if(WIN32 AND MSVC)
    target_compile_options(runtime_tests PRIVATE /MD$<$<CONFIG:Debug>:d>)
endif()

# Set XR_RUNTIME_JSON to use our runtime with mock service
gtest_discover_tests(runtime_tests
    PROPERTIES
        ENVIRONMENT "XR_RUNTIME_JSON=${CMAKE_BINARY_DIR}/bin/ox_openxr.json"
)

# Set output directory
set_target_properties(runtime_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/tests
)

# Copy ox.dll to test directory on Windows for runtime linking
if(WIN32)
    add_custom_command(TARGET runtime_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ox>
        $<TARGET_FILE_DIR:runtime_tests>
    )
endif()
