cmake_minimum_required(VERSION 3.27)

project(ox VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(OX_RUNTIME_REPO "" CACHE PATH "Local path to ox-runtime; uses FetchContent when empty")
set(OX_SIM_DRIVER_REPO "" CACHE PATH "Local path to ox-sim-driver; uses FetchContent when empty")
set(OX_IPC_PROXY_REPO "" CACHE PATH "Local path to ox-ipc-proxy; uses FetchContent when empty")

if(WIN32 AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(NOT WIN32)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)
endif()

if(WIN32)
    set(OX_PLATFORM_LIBS)
elseif(APPLE)
    set(OX_PLATFORM_LIBS pthread)
else()
    set(OX_PLATFORM_LIBS pthread rt dl)
endif()

function(ox_add_component content_name repo_url local_repo)
    if(local_repo)
        get_filename_component(component_source_dir "${local_repo}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        if(NOT EXISTS "${component_source_dir}/CMakeLists.txt")
            message(FATAL_ERROR "${content_name} local repo path does not contain a CMakeLists.txt: ${component_source_dir}")
        endif()

        message(STATUS "Using local ${content_name} repository: ${component_source_dir}")
        FetchContent_Declare(
            ${content_name}
            SOURCE_DIR "${component_source_dir}"
        )
    else()
        message(STATUS "Fetching ${content_name} from ${repo_url}")
        FetchContent_Declare(
            ${content_name}
            GIT_REPOSITORY "${repo_url}"
            GIT_TAG main
            GIT_SHALLOW TRUE
        )
    endif()

    FetchContent_MakeAvailable(${content_name})
endfunction()

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.17.0
    GIT_SHALLOW true
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    dylib
    GIT_REPOSITORY https://github.com/martin-olivier/dylib
    GIT_TAG v3.0.1
    GIT_SHALLOW true
)
FetchContent_MakeAvailable(dylib)

FetchContent_Declare(
    whereami
    GIT_REPOSITORY https://github.com/gpakosz/whereami.git
    GIT_TAG master
    GIT_SHALLOW true
)
FetchContent_MakeAvailable(whereami)

add_library(whereami STATIC ${whereami_SOURCE_DIR}/src/whereami.c)
if(NOT WIN32)
    set_target_properties(whereami PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(whereami PUBLIC ${whereami_SOURCE_DIR}/src)
if(CMAKE_DL_LIBS)
    target_link_libraries(whereami PUBLIC ${CMAKE_DL_LIBS})
endif()

ox_add_component(ox_runtime_repo https://github.com/ox-runtime/ox-runtime.git "${OX_RUNTIME_REPO}")
ox_add_component(ox_ipc_proxy_repo https://github.com/ox-runtime/ox-ipc-proxy.git "${OX_IPC_PROXY_REPO}")
ox_add_component(ox_sim_driver_repo https://github.com/ox-runtime/ox-sim-driver.git "${OX_SIM_DRIVER_REPO}")

add_executable(ox src/main.cpp)

target_include_directories(ox PRIVATE
    ${dylib_SOURCE_DIR}/include
    ${whereami_SOURCE_DIR}/src
)

target_link_libraries(ox PRIVATE
    ox_driver_api
    spdlog::spdlog
    dylib
    whereami
    ${OX_PLATFORM_LIBS}
)

target_compile_definitions(ox PRIVATE
    OX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(ox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

add_dependencies(ox ox_runtime ox_ipc_frontend ox_ipc_backend ox-simulator)

get_target_property(OX_SIM_DRIVER_OUTPUT_DIR ox-simulator RUNTIME_OUTPUT_DIRECTORY)
if(NOT OX_SIM_DRIVER_OUTPUT_DIR)
    get_target_property(OX_SIM_DRIVER_OUTPUT_DIR ox-simulator LIBRARY_OUTPUT_DIRECTORY)
endif()

if(NOT OX_SIM_DRIVER_OUTPUT_DIR)
    message(FATAL_ERROR "Could not determine the ox-sim-driver output directory")
endif()

add_custom_command(TARGET ox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ox>/drivers"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:ox>/drivers/ox_simulator"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${OX_SIM_DRIVER_OUTPUT_DIR}" "$<TARGET_FILE_DIR:ox>/drivers/ox_simulator"
    VERBATIM
)

add_custom_target(ox_bundle DEPENDS ox)

install(TARGETS ox RUNTIME DESTINATION bin)

set(MANIFEST_OUTPUT "${CMAKE_BINARY_DIR}/bin/ox_runtime.json")

# Print build completion message
add_custom_command(TARGET ox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "====================================================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Build complete!"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Service executable:     ${CMAKE_BINARY_DIR}/bin/ox${CMAKE_EXECUTABLE_SUFFIX}"
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
