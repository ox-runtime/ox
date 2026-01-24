cmake_minimum_required(VERSION 3.15)

# Fetch Google Test with shared CRT (/MD) to match OpenXR loader
include(FetchContent)

# Force GoogleTest to use /MD (dynamic runtime)
if(WIN32 AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY_BACKUP "${CMAKE_MSVC_RUNTIME_LIBRARY}")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Restore original setting
if(WIN32 AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY_BACKUP}")
endif()

# Include GoogleTest module for test discovery
include(GoogleTest)

# Add test subdirectories
add_subdirectory(runtime)

# Future test categories can be added here:
# add_subdirectory(ipc)
# add_subdirectory(driver)
# add_subdirectory(graphics)
