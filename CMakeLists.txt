cmake_minimum_required(VERSION 3.15)
project(ox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use OpenXR SDK from git submodule
set(OPENXR_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/OpenXR-SDK)

if(EXISTS ${OPENXR_SDK_DIR}/include/openxr/openxr.h)
    set(OPENXR_INCLUDE_DIR ${OPENXR_SDK_DIR}/include)
    message(STATUS "Using OpenXR SDK from submodule: ${OPENXR_SDK_DIR}")
else()
    message(FATAL_ERROR "OpenXR SDK not found at ${OPENXR_SDK_DIR}. Please initialize the submodule with: git submodule update --init")
endif()

# Build as shared library (runtime)
add_library(ox SHARED
    src/runtime.cpp
    src/logging.cpp
)

target_include_directories(ox PRIVATE
    ${OPENXR_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set output directory to build/bin (disable per-config subdirectories)
set_target_properties(ox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(ox PRIVATE
        XR_USE_PLATFORM_WIN32
        _CRT_SECURE_NO_WARNINGS
    )
    # Export symbols for Windows DLL
    target_compile_definitions(ox PRIVATE
        XR_RUNTIME_EXPORTS
    )
    set(RUNTIME_LIBRARY_NAME "ox.dll")
    set(RUNTIME_LIBRARY_PATH "./ox.dll")
elseif(UNIX)
    target_compile_definitions(ox PRIVATE
        XR_USE_PLATFORM_XLIB
    )
    # For Linux, set the SONAME
    set_target_properties(ox PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    set(RUNTIME_LIBRARY_NAME "libox.so")
    set(RUNTIME_LIBRARY_PATH "./libox.so")
endif()

# Generate the OpenXR runtime manifest JSON file
set(MANIFEST_OUTPUT "${CMAKE_BINARY_DIR}/bin/ox_openxr.json")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ox_openxr.json.in"
    "${MANIFEST_OUTPUT}"
    @ONLY
)

# Install the runtime library
install(TARGETS ox
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Print build completion message
add_custom_command(TARGET ox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Build complete!"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Runtime library: ${CMAKE_BINARY_DIR}/bin/${RUNTIME_LIBRARY_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "Manifest file:   ${MANIFEST_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "To use this runtime, set:"
    VERBATIM
)

if(WIN32)
    add_custom_command(TARGET ox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  set XR_RUNTIME_JSON=${MANIFEST_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        VERBATIM
    )
else()
    add_custom_command(TARGET ox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  export XR_RUNTIME_JSON=\"${MANIFEST_OUTPUT}\""
        COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        VERBATIM
    )
endif()
