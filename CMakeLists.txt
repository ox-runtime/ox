cmake_minimum_required(VERSION 3.15)
project(ox VERSION 0.4.1 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Ensure NDEBUG is defined for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_definitions(NDEBUG)
endif()

# Use static runtime library (/MT) on Windows for better compatibility
if(WIN32 AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Fetch OpenXR SDK
FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG release-1.1.54
)
FetchContent_MakeAvailable(openxr)

set(OPENXR_INCLUDE_DIR ${openxr_SOURCE_DIR}/include)
message(STATUS "Using OpenXR SDK from FetchContent: ${openxr_SOURCE_DIR}")

# Platform-specific link libraries
if(WIN32)
    set(SERVICE_LIBS)
else()
    set(SERVICE_LIBS pthread rt dl)
endif()

# Service executable
add_executable(ox-service
    src/service/main.cpp
    src/logging.cpp
)

target_include_directories(ox-service PRIVATE
    ${OPENXR_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ox-service PRIVATE ${SERVICE_LIBS})

# Define version macros for the service
target_compile_definitions(ox-service PRIVATE
    OX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(ox-service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# Client library (runtime DLL)
add_library(ox_runtime SHARED
    src/client/runtime.cpp
    src/client/service_connection.cpp
    src/logging.cpp
)

target_include_directories(ox_runtime PRIVATE
    ${OPENXR_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Define version macros for the runtime
target_compile_definitions(ox_runtime PRIVATE
    OX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Link OpenGL library for graphics support
find_package(OpenGL REQUIRED)
target_link_libraries(ox_runtime PRIVATE OpenGL::GL)

# Find Vulkan for graphics support
find_package(Vulkan REQUIRED)
target_link_libraries(ox_runtime PRIVATE Vulkan::Vulkan)
target_include_directories(ox_runtime PRIVATE ${Vulkan_INCLUDE_DIRS})

# Set output directory to build/bin (disable per-config subdirectories)
set_target_properties(ox_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    # For Windows, ensure .lib import library is created
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(ox_runtime PRIVATE
        XR_USE_PLATFORM_WIN32
    )
    # Export symbols for Windows DLL
    target_compile_definitions(ox_runtime PRIVATE
        XR_RUNTIME_EXPORTS
    )
    set(RUNTIME_LIBRARY_NAME "ox_runtime.dll")
    set(RUNTIME_LIBRARY_PATH "./ox_runtime.dll")
elseif(UNIX)
    target_compile_definitions(ox_runtime PRIVATE
        XR_USE_PLATFORM_XLIB
    )
    # For Linux, set the SONAME
    set_target_properties(ox_runtime PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    set(RUNTIME_LIBRARY_NAME "libox_runtime.so")
    set(RUNTIME_LIBRARY_PATH "./libox_runtime.so")
endif()

# Generate the OpenXR runtime manifest JSON file
set(MANIFEST_OUTPUT "${CMAKE_BINARY_DIR}/bin/ox_openxr.json")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ox_openxr.json.in"
    "${MANIFEST_OUTPUT}"
    @ONLY
)

# Install the runtime library and service
install(TARGETS ox_runtime ox-service
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Print build completion message
add_custom_command(TARGET ox_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Build complete!"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Runtime library: ${CMAKE_BINARY_DIR}/bin/${RUNTIME_LIBRARY_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "Service executable:     ${CMAKE_BINARY_DIR}/bin/ox-service${CMAKE_EXECUTABLE_SUFFIX}"
    COMMAND ${CMAKE_COMMAND} -E echo "Manifest file:   ${MANIFEST_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "To use this runtime, set:"
    VERBATIM
)

if(WIN32)
    add_custom_command(TARGET ox_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  set XR_RUNTIME_JSON=${MANIFEST_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        VERBATIM
    )
else()
    add_custom_command(TARGET ox_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  export XR_RUNTIME_JSON=\"${MANIFEST_OUTPUT}\""
        COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        VERBATIM
    )
endif()

# Add tests subdirectory
add_subdirectory(tests)
