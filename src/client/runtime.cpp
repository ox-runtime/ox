// Include platform headers before OpenXR
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#include <unknwn.h>
#include <windows.h>
#else
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <openxr/openxr_platform.h>

// Include OpenGL for texture creation
#ifdef _WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#include "../logging.h"
#include "service_connection.h"

using namespace ox::client;
using namespace ox::protocol;

// Export macro for Windows DLL
#ifdef _WIN32
#define RUNTIME_EXPORT __declspec(dllexport)
#else
#define RUNTIME_EXPORT __attribute__((visibility("default")))
#endif

// Instance handle management
static std::unordered_map<XrInstance, bool> g_instances;
static std::unordered_map<XrSession, XrInstance> g_sessions;
static std::unordered_map<XrSpace, XrSession> g_spaces;

// Swapchain image data
struct SwapchainData {
    std::vector<uint32_t> textureIds;  // OpenGL texture IDs
    uint32_t width;
    uint32_t height;
    int64_t format;
    bool texturesCreated;
};
static std::unordered_map<XrSwapchain, SwapchainData> g_swapchains;

// Action space metadata
struct ActionSpaceData {
    XrAction action;
    XrPath subaction_path;
};
static std::unordered_map<XrSpace, ActionSpaceData> g_action_spaces;

// Action metadata
struct ActionData {
    XrActionType type;
    XrActionSet action_set;
    std::string name;
    std::vector<XrPath> subaction_paths;
};
static std::unordered_map<XrAction, ActionData> g_actions;

// Path tracking - bidirectional mapping between paths and strings
static std::unordered_map<XrPath, std::string> g_path_to_string;
static std::unordered_map<std::string, XrPath> g_string_to_path;

// Action binding metadata - maps binding path to action
struct BindingData {
    XrAction action;
    XrPath subaction_path;  // Which hand (left/right) or XR_NULL_PATH for no subaction
};
static std::unordered_map<XrPath, BindingData> g_bindings;

// Interaction profile tracking
static XrPath g_current_interaction_profile = XR_NULL_PATH;
static std::vector<std::string> g_suggested_profiles;

static std::mutex g_instance_mutex;

// Safe string copy helper - modern C++17+ replacement for strncpy
inline void safe_copy_string(char* dest, size_t dest_size, std::string_view src) {
    if (dest_size == 0) return;
    const size_t copy_len = std::min(src.size(), dest_size - 1);
    std::copy_n(src.data(), copy_len, dest);
    dest[copy_len] = '\0';
}

// Forward declare all functions
XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                     PFN_xrVoidFunction* function);

// Function map for xrGetInstanceProcAddr
static std::unordered_map<std::string, PFN_xrVoidFunction> g_clientFunctionMap;

static void InitializeFunctionMap();

// xrEnumerateApiLayerProperties
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(uint32_t propertyCapacityInput,
                                                             uint32_t* propertyCountOutput,
                                                             XrApiLayerProperties* properties) {
    LOG_DEBUG("xrEnumerateApiLayerProperties called");
    if (propertyCountOutput) {
        *propertyCountOutput = 0;
    }
    return XR_SUCCESS;
}

// xrEnumerateInstanceExtensionProperties
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(const char* layerName,
                                                                      uint32_t propertyCapacityInput,
                                                                      uint32_t* propertyCountOutput,
                                                                      XrExtensionProperties* properties) {
    LOG_DEBUG("xrEnumerateInstanceExtensionProperties called");
    const char* extensions[] = {"XR_KHR_opengl_enable"};
    const uint32_t extensionCount = sizeof(extensions) / sizeof(extensions[0]);

    if (propertyCountOutput) {
        *propertyCountOutput = extensionCount;
    }

    if (propertyCapacityInput == 0) {
        return XR_SUCCESS;
    }

    if (!properties) {
        LOG_ERROR("xrEnumerateInstanceExtensionProperties: Null properties");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    uint32_t count = propertyCapacityInput < extensionCount ? propertyCapacityInput : extensionCount;
    for (uint32_t i = 0; i < count; i++) {
        properties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
        properties[i].next = nullptr;
        properties[i].extensionVersion = 1;
        safe_copy_string(properties[i].extensionName, XR_MAX_EXTENSION_NAME_SIZE, extensions[i]);
    }

    return XR_SUCCESS;
}

// xrCreateInstance
XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) {
    LOG_DEBUG("xrCreateInstance called");
    if (!createInfo || !instance) {
        LOG_ERROR("xrCreateInstance: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Initialize function map
    if (g_clientFunctionMap.empty()) {
        InitializeFunctionMap();
    }

    // Connect to service
    if (!ServiceConnection::Instance().Connect()) {
        LOG_ERROR("Failed to connect to service");
        return XR_ERROR_RUNTIME_FAILURE;
    }

    // Create instance handle
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    uint64_t handle = ServiceConnection::Instance().AllocateHandle(HandleType::INSTANCE);
    if (handle == 0) {
        LOG_ERROR("Failed to allocate instance handle from service");
        return XR_ERROR_RUNTIME_FAILURE;
    }

    XrInstance newInstance = (XrInstance)(uintptr_t)handle;
    g_instances[newInstance] = true;
    *instance = newInstance;

    LOG_INFO("OpenXR instance created successfully");
    return XR_SUCCESS;
}

// xrDestroyInstance
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(XrInstance instance) {
    LOG_DEBUG("xrDestroyInstance called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        LOG_ERROR("xrDestroyInstance: Invalid instance handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    g_instances.erase(it);

    // Disconnect from service if no more instances
    if (g_instances.empty()) {
        ServiceConnection::Instance().Disconnect();
    }

    LOG_INFO("OpenXR instance destroyed");
    return XR_SUCCESS;
}

// xrGetInstanceProperties
XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) {
    LOG_DEBUG("xrGetInstanceProperties called");
    if (!instanceProperties) {
        LOG_ERROR("xrGetInstanceProperties: Null instanceProperties");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_instances.find(instance) == g_instances.end()) {
        LOG_ERROR("xrGetInstanceProperties: Invalid instance handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    // Get runtime properties from cached metadata
    auto& props = ServiceConnection::Instance().GetRuntimeProperties();
    uint32_t version =
        XR_MAKE_VERSION(props.runtime_version_major, props.runtime_version_minor, props.runtime_version_patch);
    instanceProperties->runtimeVersion = version;
    safe_copy_string(instanceProperties->runtimeName, XR_MAX_RUNTIME_NAME_SIZE, props.runtime_name);

    return XR_SUCCESS;
}

// xrPollEvent - returns session state change events
XRAPI_ATTR XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
    LOG_DEBUG("xrPollEvent called");
    if (!eventData) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Get next event from service
    SessionStateEvent service_event;
    if (ServiceConnection::Instance().GetNextEvent(service_event)) {
        XrEventDataSessionStateChanged* stateEvent = (XrEventDataSessionStateChanged*)eventData;
        stateEvent->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
        stateEvent->next = nullptr;
        stateEvent->session = (XrSession)(uintptr_t)service_event.session_handle;
        stateEvent->time = service_event.timestamp;

        // Convert service SessionState to XrSessionState
        switch (service_event.state) {
            case SessionState::IDLE:
                stateEvent->state = XR_SESSION_STATE_IDLE;
                break;
            case SessionState::READY:
                stateEvent->state = XR_SESSION_STATE_READY;
                break;
            case SessionState::SYNCHRONIZED:
                stateEvent->state = XR_SESSION_STATE_SYNCHRONIZED;
                break;
            case SessionState::VISIBLE:
                stateEvent->state = XR_SESSION_STATE_VISIBLE;
                break;
            case SessionState::FOCUSED:
                stateEvent->state = XR_SESSION_STATE_FOCUSED;
                break;
            case SessionState::STOPPING:
                stateEvent->state = XR_SESSION_STATE_STOPPING;
                break;
            case SessionState::EXITING:
                stateEvent->state = XR_SESSION_STATE_EXITING;
                break;
            default:
                stateEvent->state = XR_SESSION_STATE_UNKNOWN;
                break;
        }

        LOG_INFO("Session state event from service");
        return XR_SUCCESS;
    }

    return XR_EVENT_UNAVAILABLE;
}

// String conversion maps
static const std::unordered_map<XrResult, const char*> g_resultStrings = {
    {XR_SUCCESS, "XR_SUCCESS"},
    {XR_TIMEOUT_EXPIRED, "XR_TIMEOUT_EXPIRED"},
    {XR_SESSION_LOSS_PENDING, "XR_SESSION_LOSS_PENDING"},
    {XR_EVENT_UNAVAILABLE, "XR_EVENT_UNAVAILABLE"},
    {XR_SPACE_BOUNDS_UNAVAILABLE, "XR_SPACE_BOUNDS_UNAVAILABLE"},
    {XR_SESSION_NOT_FOCUSED, "XR_SESSION_NOT_FOCUSED"},
    {XR_FRAME_DISCARDED, "XR_FRAME_DISCARDED"},
    {XR_ERROR_VALIDATION_FAILURE, "XR_ERROR_VALIDATION_FAILURE"},
    {XR_ERROR_RUNTIME_FAILURE, "XR_ERROR_RUNTIME_FAILURE"},
    {XR_ERROR_OUT_OF_MEMORY, "XR_ERROR_OUT_OF_MEMORY"},
    {XR_ERROR_API_VERSION_UNSUPPORTED, "XR_ERROR_API_VERSION_UNSUPPORTED"},
    {XR_ERROR_INITIALIZATION_FAILED, "XR_ERROR_INITIALIZATION_FAILED"},
    {XR_ERROR_FUNCTION_UNSUPPORTED, "XR_ERROR_FUNCTION_UNSUPPORTED"},
    {XR_ERROR_FEATURE_UNSUPPORTED, "XR_ERROR_FEATURE_UNSUPPORTED"},
    {XR_ERROR_EXTENSION_NOT_PRESENT, "XR_ERROR_EXTENSION_NOT_PRESENT"},
    {XR_ERROR_LIMIT_REACHED, "XR_ERROR_LIMIT_REACHED"},
    {XR_ERROR_SIZE_INSUFFICIENT, "XR_ERROR_SIZE_INSUFFICIENT"},
    {XR_ERROR_HANDLE_INVALID, "XR_ERROR_HANDLE_INVALID"},
    {XR_ERROR_INSTANCE_LOST, "XR_ERROR_INSTANCE_LOST"},
    {XR_ERROR_SESSION_RUNNING, "XR_ERROR_SESSION_RUNNING"},
    {XR_ERROR_SESSION_NOT_RUNNING, "XR_ERROR_SESSION_NOT_RUNNING"},
    {XR_ERROR_SESSION_LOST, "XR_ERROR_SESSION_LOST"},
    {XR_ERROR_SYSTEM_INVALID, "XR_ERROR_SYSTEM_INVALID"},
    {XR_ERROR_PATH_INVALID, "XR_ERROR_PATH_INVALID"},
    {XR_ERROR_PATH_COUNT_EXCEEDED, "XR_ERROR_PATH_COUNT_EXCEEDED"},
    {XR_ERROR_PATH_FORMAT_INVALID, "XR_ERROR_PATH_FORMAT_INVALID"},
    {XR_ERROR_PATH_UNSUPPORTED, "XR_ERROR_PATH_UNSUPPORTED"},
    {XR_ERROR_LAYER_INVALID, "XR_ERROR_LAYER_INVALID"},
    {XR_ERROR_LAYER_LIMIT_EXCEEDED, "XR_ERROR_LAYER_LIMIT_EXCEEDED"},
    {XR_ERROR_SWAPCHAIN_RECT_INVALID, "XR_ERROR_SWAPCHAIN_RECT_INVALID"},
    {XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED, "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED"},
    {XR_ERROR_ACTION_TYPE_MISMATCH, "XR_ERROR_ACTION_TYPE_MISMATCH"},
    {XR_ERROR_SESSION_NOT_READY, "XR_ERROR_SESSION_NOT_READY"},
    {XR_ERROR_SESSION_NOT_STOPPING, "XR_ERROR_SESSION_NOT_STOPPING"},
    {XR_ERROR_TIME_INVALID, "XR_ERROR_TIME_INVALID"},
    {XR_ERROR_REFERENCE_SPACE_UNSUPPORTED, "XR_ERROR_REFERENCE_SPACE_UNSUPPORTED"},
    {XR_ERROR_FILE_ACCESS_ERROR, "XR_ERROR_FILE_ACCESS_ERROR"},
    {XR_ERROR_FILE_CONTENTS_INVALID, "XR_ERROR_FILE_CONTENTS_INVALID"},
    {XR_ERROR_FORM_FACTOR_UNSUPPORTED, "XR_ERROR_FORM_FACTOR_UNSUPPORTED"},
    {XR_ERROR_FORM_FACTOR_UNAVAILABLE, "XR_ERROR_FORM_FACTOR_UNAVAILABLE"},
    {XR_ERROR_API_LAYER_NOT_PRESENT, "XR_ERROR_API_LAYER_NOT_PRESENT"},
    {XR_ERROR_CALL_ORDER_INVALID, "XR_ERROR_CALL_ORDER_INVALID"},
    {XR_ERROR_GRAPHICS_DEVICE_INVALID, "XR_ERROR_GRAPHICS_DEVICE_INVALID"},
    {XR_ERROR_POSE_INVALID, "XR_ERROR_POSE_INVALID"},
    {XR_ERROR_INDEX_OUT_OF_RANGE, "XR_ERROR_INDEX_OUT_OF_RANGE"},
    {XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED, "XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED"},
    {XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED, "XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED"},
    {XR_ERROR_NAME_DUPLICATED, "XR_ERROR_NAME_DUPLICATED"},
    {XR_ERROR_NAME_INVALID, "XR_ERROR_NAME_INVALID"},
    {XR_ERROR_ACTIONSET_NOT_ATTACHED, "XR_ERROR_ACTIONSET_NOT_ATTACHED"},
    {XR_ERROR_ACTIONSETS_ALREADY_ATTACHED, "XR_ERROR_ACTIONSETS_ALREADY_ATTACHED"},
    {XR_ERROR_LOCALIZED_NAME_DUPLICATED, "XR_ERROR_LOCALIZED_NAME_DUPLICATED"},
    {XR_ERROR_LOCALIZED_NAME_INVALID, "XR_ERROR_LOCALIZED_NAME_INVALID"},
    {XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING, "XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING"},
};

static const std::unordered_map<XrStructureType, const char*> g_structureTypeStrings = {
    {XR_TYPE_UNKNOWN, "XR_TYPE_UNKNOWN"},
    {XR_TYPE_API_LAYER_PROPERTIES, "XR_TYPE_API_LAYER_PROPERTIES"},
    {XR_TYPE_EXTENSION_PROPERTIES, "XR_TYPE_EXTENSION_PROPERTIES"},
    {XR_TYPE_INSTANCE_CREATE_INFO, "XR_TYPE_INSTANCE_CREATE_INFO"},
    {XR_TYPE_SYSTEM_GET_INFO, "XR_TYPE_SYSTEM_GET_INFO"},
    {XR_TYPE_SYSTEM_PROPERTIES, "XR_TYPE_SYSTEM_PROPERTIES"},
    {XR_TYPE_VIEW_LOCATE_INFO, "XR_TYPE_VIEW_LOCATE_INFO"},
    {XR_TYPE_VIEW, "XR_TYPE_VIEW"},
    {XR_TYPE_SESSION_CREATE_INFO, "XR_TYPE_SESSION_CREATE_INFO"},
    {XR_TYPE_SWAPCHAIN_CREATE_INFO, "XR_TYPE_SWAPCHAIN_CREATE_INFO"},
    {XR_TYPE_SESSION_BEGIN_INFO, "XR_TYPE_SESSION_BEGIN_INFO"},
    {XR_TYPE_VIEW_STATE, "XR_TYPE_VIEW_STATE"},
    {XR_TYPE_FRAME_END_INFO, "XR_TYPE_FRAME_END_INFO"},
    {XR_TYPE_HAPTIC_VIBRATION, "XR_TYPE_HAPTIC_VIBRATION"},
    {XR_TYPE_EVENT_DATA_BUFFER, "XR_TYPE_EVENT_DATA_BUFFER"},
    {XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING, "XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING"},
    {XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED, "XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED"},
    {XR_TYPE_ACTION_STATE_BOOLEAN, "XR_TYPE_ACTION_STATE_BOOLEAN"},
    {XR_TYPE_ACTION_STATE_FLOAT, "XR_TYPE_ACTION_STATE_FLOAT"},
    {XR_TYPE_ACTION_STATE_VECTOR2F, "XR_TYPE_ACTION_STATE_VECTOR2F"},
    {XR_TYPE_ACTION_STATE_POSE, "XR_TYPE_ACTION_STATE_POSE"},
    {XR_TYPE_ACTION_SET_CREATE_INFO, "XR_TYPE_ACTION_SET_CREATE_INFO"},
    {XR_TYPE_ACTION_CREATE_INFO, "XR_TYPE_ACTION_CREATE_INFO"},
    {XR_TYPE_INSTANCE_PROPERTIES, "XR_TYPE_INSTANCE_PROPERTIES"},
    {XR_TYPE_FRAME_WAIT_INFO, "XR_TYPE_FRAME_WAIT_INFO"},
    {XR_TYPE_COMPOSITION_LAYER_PROJECTION, "XR_TYPE_COMPOSITION_LAYER_PROJECTION"},
    {XR_TYPE_COMPOSITION_LAYER_QUAD, "XR_TYPE_COMPOSITION_LAYER_QUAD"},
    {XR_TYPE_REFERENCE_SPACE_CREATE_INFO, "XR_TYPE_REFERENCE_SPACE_CREATE_INFO"},
    {XR_TYPE_ACTION_SPACE_CREATE_INFO, "XR_TYPE_ACTION_SPACE_CREATE_INFO"},
    {XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING, "XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING"},
    {XR_TYPE_VIEW_CONFIGURATION_VIEW, "XR_TYPE_VIEW_CONFIGURATION_VIEW"},
    {XR_TYPE_SPACE_LOCATION, "XR_TYPE_SPACE_LOCATION"},
    {XR_TYPE_SPACE_VELOCITY, "XR_TYPE_SPACE_VELOCITY"},
    {XR_TYPE_FRAME_STATE, "XR_TYPE_FRAME_STATE"},
    {XR_TYPE_VIEW_CONFIGURATION_PROPERTIES, "XR_TYPE_VIEW_CONFIGURATION_PROPERTIES"},
    {XR_TYPE_FRAME_BEGIN_INFO, "XR_TYPE_FRAME_BEGIN_INFO"},
    {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, "XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW"},
    {XR_TYPE_EVENT_DATA_EVENTS_LOST, "XR_TYPE_EVENT_DATA_EVENTS_LOST"},
    {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, "XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING"},
    {XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED, "XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED"},
    {XR_TYPE_INTERACTION_PROFILE_STATE, "XR_TYPE_INTERACTION_PROFILE_STATE"},
    {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, "XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO"},
    {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, "XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO"},
    {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, "XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO"},
    {XR_TYPE_ACTION_STATE_GET_INFO, "XR_TYPE_ACTION_STATE_GET_INFO"},
    {XR_TYPE_HAPTIC_ACTION_INFO, "XR_TYPE_HAPTIC_ACTION_INFO"},
    {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, "XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO"},
    {XR_TYPE_ACTIONS_SYNC_INFO, "XR_TYPE_ACTIONS_SYNC_INFO"},
    {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO, "XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO"},
    {XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO, "XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO"},
    {XR_TYPE_COMPOSITION_LAYER_CUBE_KHR, "XR_TYPE_COMPOSITION_LAYER_CUBE_KHR"},
    {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR, "XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR"},
    {XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR, "XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR"},
    {XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR, "XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR"},
    {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR"},
    {XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR"},
    {XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR"},
    {XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR, "XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR"},
    {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, "XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR"},
    {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, "XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR"},
};

// Rest of the runtime functions (simplified versions)
XRAPI_ATTR XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value,
                                                char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    auto it = g_resultStrings.find(value);
    if (it != g_resultStrings.end()) {
        safe_copy_string(buffer, XR_MAX_RESULT_STRING_SIZE, it->second);
    } else {
        std::snprintf(buffer, XR_MAX_RESULT_STRING_SIZE, "XR_UNKNOWN_RESULT_%d", value);
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrStructureTypeToString(XrInstance instance, XrStructureType value,
                                                       char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    auto it = g_structureTypeStrings.find(value);
    if (it != g_structureTypeStrings.end()) {
        safe_copy_string(buffer, XR_MAX_STRUCTURE_NAME_SIZE, it->second);
    } else {
        std::snprintf(buffer, XR_MAX_STRUCTURE_NAME_SIZE, "XR_UNKNOWN_STRUCTURE_TYPE_%d", value);
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) {
    LOG_DEBUG("xrGetSystem called");
    if (!getInfo || !systemId) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    *systemId = 1;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties(XrInstance instance, XrSystemId systemId,
                                                     XrSystemProperties* properties) {
    LOG_DEBUG("xrGetSystemProperties called");
    if (!properties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Get system properties from cached metadata
    auto& sys_props = ServiceConnection::Instance().GetSystemProperties();
    properties->systemId = systemId;
    safe_copy_string(properties->systemName, XR_MAX_SYSTEM_NAME_SIZE, sys_props.system_name);
    properties->graphicsProperties.maxSwapchainImageWidth = sys_props.max_swapchain_width;
    properties->graphicsProperties.maxSwapchainImageHeight = sys_props.max_swapchain_height;
    properties->graphicsProperties.maxLayerCount = sys_props.max_layer_count;
    properties->trackingProperties.orientationTracking = sys_props.orientation_tracking;
    properties->trackingProperties.positionTracking = sys_props.position_tracking;

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId,
                                                             uint32_t viewConfigurationTypeCapacityInput,
                                                             uint32_t* viewConfigurationTypeCountOutput,
                                                             XrViewConfigurationType* viewConfigurationTypes) {
    LOG_DEBUG("xrEnumerateViewConfigurations called");
    const XrViewConfigurationType configs[] = {XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

    if (viewConfigurationTypeCountOutput) {
        *viewConfigurationTypeCountOutput = 1;
    }

    if (viewConfigurationTypeCapacityInput > 0 && viewConfigurationTypes) {
        viewConfigurationTypes[0] = configs[0];
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetViewConfigurationProperties(
    XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType,
    XrViewConfigurationProperties* configurationProperties) {
    LOG_DEBUG("xrGetViewConfigurationProperties called");
    if (!configurationProperties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    configurationProperties->viewConfigurationType = viewConfigurationType;
    configurationProperties->fovMutable = XR_FALSE;

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId,
                                                                 XrViewConfigurationType viewConfigurationType,
                                                                 uint32_t viewCapacityInput, uint32_t* viewCountOutput,
                                                                 XrViewConfigurationView* views) {
    LOG_DEBUG("xrEnumerateViewConfigurationViews called");

    if (viewCountOutput) {
        *viewCountOutput = 2;  // Stereo
    }

    if (viewCapacityInput > 0 && views) {
        // Get view configurations from cached metadata
        auto& view_configs = ServiceConnection::Instance().GetViewConfigurations();

        for (uint32_t i = 0; i < 2 && i < viewCapacityInput; i++) {
            views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

            auto& config = view_configs.views[i];
            views[i].recommendedImageRectWidth = config.recommended_width;
            views[i].maxImageRectWidth = config.recommended_width * 2;
            views[i].recommendedImageRectHeight = config.recommended_height;
            views[i].maxImageRectHeight = config.recommended_height * 2;
            views[i].recommendedSwapchainSampleCount = config.recommended_sample_count;
            views[i].maxSwapchainSampleCount = config.max_sample_count;
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes(XrInstance instance, XrSystemId systemId,
                                                                XrViewConfigurationType viewConfigurationType,
                                                                uint32_t environmentBlendModeCapacityInput,
                                                                uint32_t* environmentBlendModeCountOutput,
                                                                XrEnvironmentBlendMode* environmentBlendModes) {
    LOG_DEBUG("xrEnumerateEnvironmentBlendModes called");

    if (environmentBlendModeCountOutput) {
        *environmentBlendModeCountOutput = 1;
    }

    if (environmentBlendModeCapacityInput > 0 && environmentBlendModes) {
        environmentBlendModes[0] = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo,
                                               XrSession* session) {
    LOG_DEBUG("xrCreateSession called");
    if (!createInfo || !session) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Check for graphics binding in the next chain (required for rendering)
    const void* next = createInfo->next;
    bool hasGraphicsBinding = false;
    while (next) {
        const XrBaseInStructure* header = reinterpret_cast<const XrBaseInStructure*>(next);
        if (header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR ||
            header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR ||
            header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR ||
            header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR) {
            hasGraphicsBinding = true;
            LOG_DEBUG("xrCreateSession: OpenGL graphics binding detected");
            break;
        }
        next = header->next;
    }

    if (hasGraphicsBinding) {
        LOG_INFO("Session created with graphics binding (rendering not implemented yet)");
    }

    // Service will create session and return handle via CREATE_SESSION message
    ServiceConnection::Instance().SendRequest(MessageType::CREATE_SESSION);

    // The service returns the session handle in the response, but for now
    // we'll get it from shared memory after the state transitions
    auto* shared_data = ServiceConnection::Instance().GetSharedData();
    if (!shared_data) {
        LOG_ERROR("xrCreateSession: No service connection");
        return XR_ERROR_RUNTIME_FAILURE;
    }

    // Wait briefly for service to set the session handle
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint64_t handle = shared_data->active_session_handle.load(std::memory_order_acquire);

    if (handle == 0) {
        LOG_ERROR("xrCreateSession: Service did not create session");
        return XR_ERROR_RUNTIME_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    XrSession newSession = (XrSession)(uintptr_t)handle;
    g_sessions[newSession] = instance;
    *session = newSession;

    LOG_INFO("Session created");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroySession(XrSession session) {
    LOG_DEBUG("xrDestroySession called");

    ServiceConnection::Instance().SendRequest(MessageType::DESTROY_SESSION);

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    g_sessions.erase(session);

    LOG_INFO("Session destroyed");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
    LOG_DEBUG("xrBeginSession called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEndSession(XrSession session) {
    LOG_DEBUG("xrEndSession called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrRequestExitSession(XrSession session) {
    LOG_DEBUG("xrRequestExitSession called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateReferenceSpaces(XrSession session, uint32_t spaceCapacityInput,
                                                          uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces) {
    LOG_DEBUG("xrEnumerateReferenceSpaces called");
    const XrReferenceSpaceType supportedSpaces[] = {XR_REFERENCE_SPACE_TYPE_VIEW, XR_REFERENCE_SPACE_TYPE_LOCAL,
                                                    XR_REFERENCE_SPACE_TYPE_STAGE};

    if (spaceCountOutput) {
        *spaceCountOutput = 3;
    }

    if (spaceCapacityInput > 0 && spaces) {
        uint32_t count = spaceCapacityInput < 3 ? spaceCapacityInput : 3;
        for (uint32_t i = 0; i < count; i++) {
            spaces[i] = supportedSpaces[i];
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo,
                                                      XrSpace* space) {
    LOG_DEBUG("xrCreateReferenceSpace called");
    if (!createInfo || !space) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    uint64_t handle = ServiceConnection::Instance().AllocateHandle(HandleType::SPACE);
    if (handle == 0) {
        LOG_ERROR("Failed to allocate space handle from service");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    XrSpace newSpace = (XrSpace)(uintptr_t)handle;
    g_spaces[newSpace] = session;
    *space = newSpace;

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroySpace(XrSpace space) {
    LOG_DEBUG("xrDestroySpace called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    g_spaces.erase(space);
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) {
    LOG_DEBUG("xrLocateSpace called");
    if (!location) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);

    // Check if this is an action space
    auto it = g_action_spaces.find(space);
    if (it != g_action_spaces.end()) {
        // This is an action space - get controller pose from shared memory
        auto* shared_data = ServiceConnection::Instance().GetSharedData();
        if (!shared_data) {
            location->locationFlags = 0;
            return XR_SUCCESS;
        }

        // Determine controller index from subaction path
        // /user/hand/left = 0, /user/hand/right = 1
        int controller_index = -1;
        if (it->second.subaction_path != 0) {
            // Simple heuristic: use lower bit of path handle
            // In a real implementation, would parse the path string
            controller_index = (it->second.subaction_path & 1);
        }

        if (controller_index >= 0 && controller_index < 2) {
            // Read controller pose from shared memory (indices 0-1 for controllers)
            auto& pose_data = shared_data->frame_state.controller_poses[controller_index];
            uint32_t flags = pose_data.flags.load(std::memory_order_acquire);

            if (flags & 1) {  // Active flag
                location->locationFlags =
                    XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT |
                    XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;

                location->pose.position.x = pose_data.position[0];
                location->pose.position.y = pose_data.position[1];
                location->pose.position.z = pose_data.position[2];

                location->pose.orientation.x = pose_data.orientation[0];
                location->pose.orientation.y = pose_data.orientation[1];
                location->pose.orientation.z = pose_data.orientation[2];
                location->pose.orientation.w = pose_data.orientation[3];
            } else {
                // Controller not active
                location->locationFlags = 0;
            }
        } else {
            location->locationFlags = 0;
        }

        return XR_SUCCESS;
    }

    // Regular reference space
    location->locationFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT |
                              XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
    location->pose.orientation.x = 0.0f;
    location->pose.orientation.y = 0.0f;
    location->pose.orientation.z = 0.0f;
    location->pose.orientation.w = 1.0f;
    location->pose.position.x = 0.0f;
    location->pose.position.y = 1.6f;  // Eye height
    location->pose.position.z = 0.0f;

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo,
                                           XrFrameState* frameState) {
    LOG_DEBUG("xrWaitFrame called");
    if (!frameState) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Read frame data from shared memory
    auto* shared_data = ServiceConnection::Instance().GetSharedData();
    if (shared_data) {
        frameState->predictedDisplayTime =
            shared_data->frame_state.predicted_display_time.load(std::memory_order_acquire);
        frameState->predictedDisplayPeriod = 11111111;  // ~90 FPS
    } else {
        frameState->predictedDisplayTime = 0;
        frameState->predictedDisplayPeriod = 11111111;
    }

    frameState->shouldRender = XR_TRUE;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
    LOG_DEBUG("xrBeginFrame called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
    LOG_DEBUG("xrEndFrame called");
    if (!frameEndInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // We're not rendering anything to the device yet, but we need to safely
    // handle the layer information to prevent crashes
    if (frameEndInfo->layerCount > 0 && frameEndInfo->layers) {
        LOG_DEBUG("xrEndFrame: Received layers (ignoring for now)");
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo,
                                             XrViewState* viewState, uint32_t viewCapacityInput,
                                             uint32_t* viewCountOutput, XrView* views) {
    LOG_DEBUG("xrLocateViews called");
    if (!viewLocateInfo || !viewState || !viewCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    *viewCountOutput = 2;

    if (viewCapacityInput == 0) {
        return XR_SUCCESS;
    }

    viewState->viewStateFlags = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;

    // Read pose data from shared memory
    auto* shared_data = ServiceConnection::Instance().GetSharedData();
    if (views && viewCapacityInput >= 2 && shared_data) {
        uint32_t view_count = shared_data->frame_state.view_count.load(std::memory_order_acquire);

        for (uint32_t i = 0; i < 2 && i < viewCapacityInput; i++) {
            views[i].type = XR_TYPE_VIEW;
            views[i].next = nullptr;

            auto& view_data = shared_data->frame_state.views[i];

            views[i].pose.position.x = view_data.pose.position[0];
            views[i].pose.position.y = view_data.pose.position[1];
            views[i].pose.position.z = view_data.pose.position[2];

            views[i].pose.orientation.x = view_data.pose.orientation[0];
            views[i].pose.orientation.y = view_data.pose.orientation[1];
            views[i].pose.orientation.z = view_data.pose.orientation[2];
            views[i].pose.orientation.w = view_data.pose.orientation[3];

            views[i].fov.angleLeft = view_data.fov[0];
            views[i].fov.angleRight = view_data.fov[1];
            views[i].fov.angleUp = view_data.fov[2];
            views[i].fov.angleDown = view_data.fov[3];
        }
    }

    return XR_SUCCESS;
}

// Action system stubs
XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo,
                                                 XrActionSet* actionSet) {
    LOG_DEBUG("xrCreateActionSet called");
    if (!createInfo || !actionSet) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    uint64_t handle = ServiceConnection::Instance().AllocateHandle(HandleType::ACTION_SET);
    if (handle == 0) {
        LOG_ERROR("Failed to allocate action set handle from service");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    *actionSet = (XrActionSet)(uintptr_t)handle;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet actionSet) {
    LOG_DEBUG("xrDestroyActionSet called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo,
                                              XrAction* action) {
    LOG_DEBUG("xrCreateAction called");
    if (!createInfo || !action) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    uint64_t handle = ServiceConnection::Instance().AllocateHandle(HandleType::ACTION);
    if (handle == 0) {
        LOG_ERROR("Failed to allocate action handle from service");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    *action = (XrAction)(uintptr_t)handle;

    // Store action metadata
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    ActionData& data = g_actions[*action];
    data.type = createInfo->actionType;
    data.action_set = actionSet;
    data.name = createInfo->actionName;
    if (createInfo->countSubactionPaths > 0 && createInfo->subactionPaths) {
        data.subaction_paths.assign(createInfo->subactionPaths,
                                    createInfo->subactionPaths + createInfo->countSubactionPaths);
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyAction(XrAction action) {
    LOG_DEBUG("xrDestroyAction called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSuggestInteractionProfileBindings(
    XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    LOG_DEBUG("xrSuggestInteractionProfileBindings called");
    if (!suggestedBindings) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Convert XrPath to string for storage
    char profile_path[256];
    uint32_t out_len = 0;
    XrResult result =
        xrPathToString(instance, suggestedBindings->interactionProfile, sizeof(profile_path), &out_len, profile_path);
    if (result == XR_SUCCESS) {
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        // Store this profile as a suggested profile
        std::string profile_str(profile_path);
        if (std::find(g_suggested_profiles.begin(), g_suggested_profiles.end(), profile_str) ==
            g_suggested_profiles.end()) {
            g_suggested_profiles.push_back(profile_str);
            LOG_DEBUG(("Suggested profile: " + profile_str).c_str());
        }

        // Store the bindings for this profile
        for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
            const XrActionSuggestedBinding& binding = suggestedBindings->suggestedBindings[i];
            XrPath binding_path = binding.binding;

            // Find the subaction path from the action metadata
            XrPath subaction_path = XR_NULL_PATH;
            auto action_it = g_actions.find(binding.action);
            if (action_it != g_actions.end()) {
                // For actions with subaction paths, we need to extract it from the binding path
                // The binding path format is /user/hand/{left|right}/input/...
                // We'll store all bindings and determine subaction at query time
                if (!action_it->second.subaction_paths.empty()) {
                    char binding_path_str[256];
                    uint32_t len = 0;
                    xrPathToString(instance, binding_path, sizeof(binding_path_str), &len, binding_path_str);
                    std::string path_str(binding_path_str);
                    // Check if path contains /user/hand/left or /user/hand/right
                    if (path_str.find("/user/hand/left") != std::string::npos) {
                        subaction_path = action_it->second.subaction_paths[0];  // Assuming first is left
                    } else if (path_str.find("/user/hand/right") != std::string::npos) {
                        subaction_path = action_it->second.subaction_paths.size() > 1
                                             ? action_it->second.subaction_paths[1]
                                             : action_it->second.subaction_paths[0];
                    }
                }
            }

            g_bindings[binding_path] = BindingData{binding.action, subaction_path};
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession session,
                                                         const XrSessionActionSetsAttachInfo* attachInfo) {
    LOG_DEBUG("xrAttachSessionActionSets called");

    // When action sets are attached, select the best matching interaction profile
    std::lock_guard<std::mutex> lock(g_instance_mutex);

    // Get driver-supported profiles from service
    const auto& driver_profiles = ServiceConnection::Instance().GetInteractionProfiles();

    // Try to find a match between suggested profiles and driver-supported profiles
    for (const auto& suggested : g_suggested_profiles) {
        for (uint32_t i = 0; i < driver_profiles.profile_count && i < 8; i++) {
            std::string driver_profile(driver_profiles.profiles[i]);
            if (suggested == driver_profile) {
                // Found a match! Set this as the active profile
                auto it = g_sessions.find(session);
                if (it != g_sessions.end()) {
                    XrInstance instance = it->second;
                    XrResult result = xrStringToPath(instance, suggested.c_str(), &g_current_interaction_profile);
                    if (result == XR_SUCCESS) {
                        LOG_INFO(("Activated interaction profile: " + suggested).c_str());
                        return XR_SUCCESS;
                    }
                }
            }
        }
    }

    // If no match found but driver has profiles, use the first one
    if (driver_profiles.profile_count > 0) {
        auto it = g_sessions.find(session);
        if (it != g_sessions.end()) {
            XrInstance instance = it->second;
            std::string profile(driver_profiles.profiles[0]);
            XrResult result = xrStringToPath(instance, profile.c_str(), &g_current_interaction_profile);
            if (result == XR_SUCCESS) {
                LOG_INFO(("Activated default driver profile: " + profile).c_str());
            }
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetCurrentInteractionProfile(XrSession session, XrPath topLevelUserPath,
                                                              XrInteractionProfileState* interactionProfile) {
    LOG_DEBUG("xrGetCurrentInteractionProfile called");
    if (!interactionProfile) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    interactionProfile->interactionProfile = g_current_interaction_profile;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
    LOG_DEBUG("xrSyncActions called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo* getInfo,
                                                       XrActionStateBoolean* state) {
    LOG_DEBUG("xrGetActionStateBoolean called");
    if (!state || !getInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Initialize to inactive state
    state->currentState = XR_FALSE;
    state->changedSinceLastSync = XR_FALSE;
    state->lastChangeTime = 0;
    state->isActive = XR_FALSE;

    std::lock_guard<std::mutex> lock(g_instance_mutex);

    // Get instance from session
    auto session_it = g_sessions.find(session);
    if (session_it == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    XrInstance instance = session_it->second;

    // Find the action metadata
    auto action_it = g_actions.find(getInfo->action);
    if (action_it == g_actions.end()) {
        return XR_SUCCESS;  // Action not found, return inactive
    }

    // Find a binding for this action + subaction path
    for (const auto& [binding_path, binding_data] : g_bindings) {
        if (binding_data.action != getInfo->action) {
            continue;  // Not our action
        }

        // Check if subaction path matches (or no subaction requested)
        if (getInfo->subactionPath != XR_NULL_PATH && binding_data.subaction_path != XR_NULL_PATH &&
            binding_data.subaction_path != getInfo->subactionPath) {
            continue;  // Wrong subaction (wrong hand)
        }

        // Convert binding path to string to extract component path
        char binding_path_str[256];
        uint32_t len = 0;
        xrPathToString(instance, binding_path, sizeof(binding_path_str), &len, binding_path_str);

        // Determine controller index from path (/user/hand/left = 0, /user/hand/right = 1)
        uint32_t controller_index = 0;
        std::string path_str(binding_path_str);
        if (path_str.find("/user/hand/right") != std::string::npos) {
            controller_index = 1;
        }

        // Query the driver for component state
        ox::protocol::InputComponentStateResponse response;
        if (ServiceConnection::Instance().GetInputComponentState(controller_index, binding_path_str, 0, response)) {
            if (response.is_available) {
                state->currentState = response.boolean_value ? XR_TRUE : XR_FALSE;
                state->isActive = XR_TRUE;
                state->lastChangeTime = 0;  // TODO: track actual change time
                return XR_SUCCESS;
            }
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo,
                                                     XrActionStateFloat* state) {
    LOG_DEBUG("xrGetActionStateFloat called");
    if (!state || !getInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Initialize to inactive state
    state->currentState = 0.0f;
    state->changedSinceLastSync = XR_FALSE;
    state->lastChangeTime = 0;
    state->isActive = XR_FALSE;

    std::lock_guard<std::mutex> lock(g_instance_mutex);

    // Get instance from session
    auto session_it = g_sessions.find(session);
    if (session_it == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    XrInstance instance = session_it->second;

    // Find the action metadata
    auto action_it = g_actions.find(getInfo->action);
    if (action_it == g_actions.end()) {
        return XR_SUCCESS;  // Action not found, return inactive
    }

    // Find a binding for this action + subaction path
    for (const auto& [binding_path, binding_data] : g_bindings) {
        if (binding_data.action != getInfo->action) {
            continue;  // Not our action
        }

        // Check if subaction path matches (or no subaction requested)
        if (getInfo->subactionPath != XR_NULL_PATH && binding_data.subaction_path != XR_NULL_PATH &&
            binding_data.subaction_path != getInfo->subactionPath) {
            continue;  // Wrong subaction (wrong hand)
        }

        // Convert binding path to string to extract component path
        char binding_path_str[256];
        uint32_t len = 0;
        xrPathToString(instance, binding_path, sizeof(binding_path_str), &len, binding_path_str);

        // Determine controller index from path (/user/hand/left = 0, /user/hand/right = 1)
        uint32_t controller_index = 0;
        std::string path_str(binding_path_str);
        if (path_str.find("/user/hand/right") != std::string::npos) {
            controller_index = 1;
        }

        // Query the driver for component state
        ox::protocol::InputComponentStateResponse response;
        if (ServiceConnection::Instance().GetInputComponentState(controller_index, binding_path_str, 0, response)) {
            if (response.is_available) {
                state->currentState = response.float_value;
                state->isActive = XR_TRUE;
                state->lastChangeTime = 0;  // TODO: track actual change time
                return XR_SUCCESS;
            }
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo,
                                                        XrActionStateVector2f* state) {
    LOG_DEBUG("xrGetActionStateVector2f called");
    if (!state || !getInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Initialize to inactive state
    state->currentState.x = 0.0f;
    state->currentState.y = 0.0f;
    state->changedSinceLastSync = XR_FALSE;
    state->lastChangeTime = 0;
    state->isActive = XR_FALSE;

    std::lock_guard<std::mutex> lock(g_instance_mutex);

    // Get instance from session
    auto session_it = g_sessions.find(session);
    if (session_it == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    XrInstance instance = session_it->second;

    // Find the action metadata
    auto action_it = g_actions.find(getInfo->action);
    if (action_it == g_actions.end()) {
        return XR_SUCCESS;  // Action not found, return inactive
    }

    // Find a binding for this action + subaction path
    for (const auto& [binding_path, binding_data] : g_bindings) {
        if (binding_data.action != getInfo->action) {
            continue;  // Not our action
        }

        // Check if subaction path matches (or no subaction requested)
        if (getInfo->subactionPath != XR_NULL_PATH && binding_data.subaction_path != XR_NULL_PATH &&
            binding_data.subaction_path != getInfo->subactionPath) {
            continue;  // Wrong subaction (wrong hand)
        }

        // Convert binding path to string to extract component path
        char binding_path_str[256];
        uint32_t len = 0;
        xrPathToString(instance, binding_path, sizeof(binding_path_str), &len, binding_path_str);

        // Determine controller index from path (/user/hand/left = 0, /user/hand/right = 1)
        uint32_t controller_index = 0;
        std::string path_str(binding_path_str);
        if (path_str.find("/user/hand/right") != std::string::npos) {
            controller_index = 1;
        }

        // Query the driver for component state
        ox::protocol::InputComponentStateResponse response;
        if (ServiceConnection::Instance().GetInputComponentState(controller_index, binding_path_str, 0, response)) {
            if (response.is_available) {
                state->currentState.x = response.x;
                state->currentState.y = response.y;
                state->isActive = XR_TRUE;
                state->lastChangeTime = 0;  // TODO: track actual change time
                return XR_SUCCESS;
            }
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStatePose(XrSession session, const XrActionStateGetInfo* getInfo,
                                                    XrActionStatePose* state) {
    LOG_DEBUG("xrGetActionStatePose called");
    if (!state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    state->isActive = XR_TRUE;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo* createInfo,
                                                   XrSpace* space) {
    LOG_DEBUG("xrCreateActionSpace called");
    if (!createInfo || !space) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    uint64_t handle = ServiceConnection::Instance().AllocateHandle(HandleType::SPACE);
    if (handle == 0) {
        LOG_ERROR("Failed to allocate action space handle from service");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    XrSpace newSpace = (XrSpace)(uintptr_t)handle;
    g_spaces[newSpace] = session;

    // Store action space metadata
    ActionSpaceData data;
    data.action = createInfo->action;
    data.subaction_path = createInfo->subactionPath;
    g_action_spaces[newSpace] = data;

    *space = newSpace;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetReferenceSpaceBoundsRect(XrSession session, XrReferenceSpaceType referenceSpaceType,
                                                             XrExtent2Df* bounds) {
    LOG_DEBUG("xrGetReferenceSpaceBoundsRect called");
    if (!bounds) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Return failure to indicate bounds are not available
    return XR_SPACE_BOUNDS_UNAVAILABLE;
}

XRAPI_ATTR XrResult XRAPI_CALL
xrEnumerateBoundSourcesForAction(XrSession session, const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
                                 uint32_t sourceCapacityInput, uint32_t* sourceCountOutput, XrPath* sources) {
    LOG_DEBUG("xrEnumerateBoundSourcesForAction called");
    if (!enumerateInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    if (sourceCountOutput) {
        *sourceCountOutput = 0;
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetInputSourceLocalizedName(XrSession session,
                                                             const XrInputSourceLocalizedNameGetInfo* getInfo,
                                                             uint32_t bufferCapacityInput, uint32_t* bufferCountOutput,
                                                             char* buffer) {
    LOG_DEBUG("xrGetInputSourceLocalizedName called");
    if (!getInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    const char* name = "Unknown";
    uint32_t len = strlen(name) + 1;
    if (bufferCountOutput) {
        *bufferCountOutput = len;
    }
    if (bufferCapacityInput > 0 && buffer) {
        safe_copy_string(buffer, bufferCapacityInput, name);
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrApplyHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo,
                                                     const XrHapticBaseHeader* hapticFeedback) {
    LOG_DEBUG("xrApplyHapticFeedback called");
    if (!hapticActionInfo || !hapticFeedback) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Haptic feedback not implemented yet
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo) {
    LOG_DEBUG("xrStopHapticFeedback called");
    if (!hapticActionInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Haptic feedback not implemented yet
    return XR_SUCCESS;
}

// Swapchain functions
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput,
                                                           uint32_t* formatCountOutput, int64_t* formats) {
    LOG_DEBUG("xrEnumerateSwapchainFormats called");
    const int64_t supportedFormats[] = {0x1908, 0x8058};  // GL_RGBA, GL_RGBA8

    if (formatCountOutput) {
        *formatCountOutput = 2;
    }

    if (formatCapacityInput > 0 && formats) {
        uint32_t count = formatCapacityInput < 2 ? formatCapacityInput : 2;
        for (uint32_t i = 0; i < count; i++) {
            formats[i] = supportedFormats[i];
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo,
                                                 XrSwapchain* swapchain) {
    LOG_DEBUG("xrCreateSwapchain called");
    if (!createInfo || !swapchain) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    uint64_t handle = ServiceConnection::Instance().AllocateHandle(HandleType::SWAPCHAIN);
    if (handle == 0) {
        LOG_ERROR("Failed to allocate swapchain handle from service");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    *swapchain = (XrSwapchain)(uintptr_t)handle;

    // Store swapchain data for later texture creation
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    SwapchainData data;
    data.width = createInfo->width;
    data.height = createInfo->height;
    data.format = createInfo->format;
    data.texturesCreated = false;
    g_swapchains[*swapchain] = data;

    LOG_DEBUG("Swapchain created successfully");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain swapchain) {
    LOG_DEBUG("xrDestroySwapchain called");

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_swapchains.find(swapchain);
    if (it != g_swapchains.end()) {
        // Delete OpenGL textures if they were created
        if (it->second.texturesCreated && !it->second.textureIds.empty()) {
            glDeleteTextures(static_cast<GLsizei>(it->second.textureIds.size()), it->second.textureIds.data());
        }
        g_swapchains.erase(it);
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput,
                                                          uint32_t* imageCountOutput,
                                                          XrSwapchainImageBaseHeader* images) {
    LOG_DEBUG("xrEnumerateSwapchainImages called");
    const uint32_t numImages = 3;

    if (imageCountOutput) {
        *imageCountOutput = numImages;
    }

    if (imageCapacityInput > 0 && images) {
        // Get or create textures for this swapchain
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        auto it = g_swapchains.find(swapchain);

        // Determine if this API type needs OpenGL textures
        // Note: XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR = 1000024002
        //       XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR = 1000023000
        // Both have the same memory layout (type, next, image) so we can handle them the same way
        XrStructureType imageType = images[0].type;
        bool needsTextures = (imageType == XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR ||
                              imageType == static_cast<XrStructureType>(1000024002));  // OPENGL_ES_KHR

        if (it != g_swapchains.end() && needsTextures && !it->second.texturesCreated) {
            // Create OpenGL textures on first enumeration
            it->second.textureIds.resize(numImages);
            glGenTextures(numImages, it->second.textureIds.data());

            // Initialize each texture with minimal settings
            for (uint32_t i = 0; i < numImages; i++) {
                glBindTexture(GL_TEXTURE_2D, it->second.textureIds[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, it->second.width, it->second.height, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            it->second.texturesCreated = true;
        }

        // Populate the image array
        // Both OpenGL and OpenGL ES use the same structure layout
        if (needsTextures) {
            // Cast to OpenGL structure (works for both GL and GLES)
            XrSwapchainImageOpenGLKHR* glImages = reinterpret_cast<XrSwapchainImageOpenGLKHR*>(images);
            for (uint32_t i = 0; i < imageCapacityInput && i < numImages; i++) {
                glImages[i].type = imageType;  // Preserve the original type
                glImages[i].next = nullptr;
                if (it != g_swapchains.end() && i < it->second.textureIds.size()) {
                    glImages[i].image = it->second.textureIds[i];
                } else {
                    glImages[i].image = 0;
                }
            }
        } else {
            // For other graphics APIs (Vulkan, D3D, etc.), just set the base header
            for (uint32_t i = 0; i < imageCapacityInput && i < numImages; i++) {
                images[i].type = imageType;
                images[i].next = nullptr;
            }
        }
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain swapchain,
                                                       const XrSwapchainImageAcquireInfo* acquireInfo,
                                                       uint32_t* index) {
    LOG_DEBUG("xrAcquireSwapchainImage called");
    if (index) {
        *index = 0;
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) {
    LOG_DEBUG("xrWaitSwapchainImage called");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain swapchain,
                                                       const XrSwapchainImageReleaseInfo* releaseInfo) {
    LOG_DEBUG("xrReleaseSwapchainImage called");
    return XR_SUCCESS;
}

// Path/string functions
XRAPI_ATTR XrResult XRAPI_CALL xrStringToPath(XrInstance instance, const char* pathString, XrPath* path) {
    LOG_DEBUG("xrStringToPath called");
    if (!pathString || !path) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Note: Caller must hold g_instance_mutex
    // Check if we've already created this path
    std::string path_str(pathString);
    auto it = g_string_to_path.find(path_str);
    if (it != g_string_to_path.end()) {
        *path = it->second;
    } else {
        // Create new path using hash
        *path = (XrPath)std::hash<std::string>{}(pathString);
        g_path_to_string[*path] = path_str;
        g_string_to_path[path_str] = *path;
    }

    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrPathToString(XrInstance instance, XrPath path, uint32_t bufferCapacityInput,
                                              uint32_t* bufferCountOutput, char* buffer) {
    LOG_DEBUG("xrPathToString called");

    // Note: Caller must hold g_instance_mutex
    // Look up the path string
    auto it = g_path_to_string.find(path);
    const char* str = (it != g_path_to_string.end()) ? it->second.c_str() : "/unknown/path";
    uint32_t len = strlen(str) + 1;

    if (bufferCountOutput) {
        *bufferCountOutput = len;
    }

    if (bufferCapacityInput > 0 && buffer) {
        safe_copy_string(buffer, bufferCapacityInput, str);
    }

    return XR_SUCCESS;
}

// OpenGL extension
XRAPI_ATTR XrResult XRAPI_CALL xrGetOpenGLGraphicsRequirementsKHR(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
    LOG_DEBUG("xrGetOpenGLGraphicsRequirementsKHR called");
    if (!graphicsRequirements) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    graphicsRequirements->minApiVersionSupported = XR_MAKE_VERSION(3, 3, 0);
    graphicsRequirements->maxApiVersionSupported = XR_MAKE_VERSION(4, 6, 0);
    return XR_SUCCESS;
}

// Function map initialization
static void InitializeFunctionMap() {
    g_clientFunctionMap["xrEnumerateApiLayerProperties"] = (PFN_xrVoidFunction)xrEnumerateApiLayerProperties;
    g_clientFunctionMap["xrEnumerateInstanceExtensionProperties"] =
        (PFN_xrVoidFunction)xrEnumerateInstanceExtensionProperties;
    g_clientFunctionMap["xrCreateInstance"] = (PFN_xrVoidFunction)xrCreateInstance;
    g_clientFunctionMap["xrDestroyInstance"] = (PFN_xrVoidFunction)xrDestroyInstance;
    g_clientFunctionMap["xrGetInstanceProperties"] = (PFN_xrVoidFunction)xrGetInstanceProperties;
    g_clientFunctionMap["xrPollEvent"] = (PFN_xrVoidFunction)xrPollEvent;
    g_clientFunctionMap["xrResultToString"] = (PFN_xrVoidFunction)xrResultToString;
    g_clientFunctionMap["xrStructureTypeToString"] = (PFN_xrVoidFunction)xrStructureTypeToString;
    g_clientFunctionMap["xrGetSystem"] = (PFN_xrVoidFunction)xrGetSystem;
    g_clientFunctionMap["xrGetSystemProperties"] = (PFN_xrVoidFunction)xrGetSystemProperties;
    g_clientFunctionMap["xrEnumerateViewConfigurations"] = (PFN_xrVoidFunction)xrEnumerateViewConfigurations;
    g_clientFunctionMap["xrGetViewConfigurationProperties"] = (PFN_xrVoidFunction)xrGetViewConfigurationProperties;
    g_clientFunctionMap["xrEnumerateViewConfigurationViews"] = (PFN_xrVoidFunction)xrEnumerateViewConfigurationViews;
    g_clientFunctionMap["xrEnumerateEnvironmentBlendModes"] = (PFN_xrVoidFunction)xrEnumerateEnvironmentBlendModes;
    g_clientFunctionMap["xrCreateSession"] = (PFN_xrVoidFunction)xrCreateSession;
    g_clientFunctionMap["xrDestroySession"] = (PFN_xrVoidFunction)xrDestroySession;
    g_clientFunctionMap["xrBeginSession"] = (PFN_xrVoidFunction)xrBeginSession;
    g_clientFunctionMap["xrEndSession"] = (PFN_xrVoidFunction)xrEndSession;
    g_clientFunctionMap["xrRequestExitSession"] = (PFN_xrVoidFunction)xrRequestExitSession;
    g_clientFunctionMap["xrEnumerateReferenceSpaces"] = (PFN_xrVoidFunction)xrEnumerateReferenceSpaces;
    g_clientFunctionMap["xrCreateReferenceSpace"] = (PFN_xrVoidFunction)xrCreateReferenceSpace;
    g_clientFunctionMap["xrDestroySpace"] = (PFN_xrVoidFunction)xrDestroySpace;
    g_clientFunctionMap["xrLocateSpace"] = (PFN_xrVoidFunction)xrLocateSpace;
    g_clientFunctionMap["xrWaitFrame"] = (PFN_xrVoidFunction)xrWaitFrame;
    g_clientFunctionMap["xrBeginFrame"] = (PFN_xrVoidFunction)xrBeginFrame;
    g_clientFunctionMap["xrEndFrame"] = (PFN_xrVoidFunction)xrEndFrame;
    g_clientFunctionMap["xrLocateViews"] = (PFN_xrVoidFunction)xrLocateViews;
    g_clientFunctionMap["xrCreateActionSet"] = (PFN_xrVoidFunction)xrCreateActionSet;
    g_clientFunctionMap["xrDestroyActionSet"] = (PFN_xrVoidFunction)xrDestroyActionSet;
    g_clientFunctionMap["xrCreateAction"] = (PFN_xrVoidFunction)xrCreateAction;
    g_clientFunctionMap["xrDestroyAction"] = (PFN_xrVoidFunction)xrDestroyAction;
    g_clientFunctionMap["xrSuggestInteractionProfileBindings"] =
        (PFN_xrVoidFunction)xrSuggestInteractionProfileBindings;
    g_clientFunctionMap["xrAttachSessionActionSets"] = (PFN_xrVoidFunction)xrAttachSessionActionSets;
    g_clientFunctionMap["xrGetCurrentInteractionProfile"] = (PFN_xrVoidFunction)xrGetCurrentInteractionProfile;
    g_clientFunctionMap["xrSyncActions"] = (PFN_xrVoidFunction)xrSyncActions;
    g_clientFunctionMap["xrGetActionStateBoolean"] = (PFN_xrVoidFunction)xrGetActionStateBoolean;
    g_clientFunctionMap["xrGetActionStateFloat"] = (PFN_xrVoidFunction)xrGetActionStateFloat;
    g_clientFunctionMap["xrGetActionStateVector2f"] = (PFN_xrVoidFunction)xrGetActionStateVector2f;
    g_clientFunctionMap["xrGetActionStatePose"] = (PFN_xrVoidFunction)xrGetActionStatePose;
    g_clientFunctionMap["xrCreateActionSpace"] = (PFN_xrVoidFunction)xrCreateActionSpace;
    g_clientFunctionMap["xrGetReferenceSpaceBoundsRect"] = (PFN_xrVoidFunction)xrGetReferenceSpaceBoundsRect;
    g_clientFunctionMap["xrEnumerateBoundSourcesForAction"] = (PFN_xrVoidFunction)xrEnumerateBoundSourcesForAction;
    g_clientFunctionMap["xrGetInputSourceLocalizedName"] = (PFN_xrVoidFunction)xrGetInputSourceLocalizedName;
    g_clientFunctionMap["xrApplyHapticFeedback"] = (PFN_xrVoidFunction)xrApplyHapticFeedback;
    g_clientFunctionMap["xrStopHapticFeedback"] = (PFN_xrVoidFunction)xrStopHapticFeedback;
    g_clientFunctionMap["xrEnumerateSwapchainFormats"] = (PFN_xrVoidFunction)xrEnumerateSwapchainFormats;
    g_clientFunctionMap["xrCreateSwapchain"] = (PFN_xrVoidFunction)xrCreateSwapchain;
    g_clientFunctionMap["xrDestroySwapchain"] = (PFN_xrVoidFunction)xrDestroySwapchain;
    g_clientFunctionMap["xrEnumerateSwapchainImages"] = (PFN_xrVoidFunction)xrEnumerateSwapchainImages;
    g_clientFunctionMap["xrAcquireSwapchainImage"] = (PFN_xrVoidFunction)xrAcquireSwapchainImage;
    g_clientFunctionMap["xrWaitSwapchainImage"] = (PFN_xrVoidFunction)xrWaitSwapchainImage;
    g_clientFunctionMap["xrReleaseSwapchainImage"] = (PFN_xrVoidFunction)xrReleaseSwapchainImage;
    g_clientFunctionMap["xrStringToPath"] = (PFN_xrVoidFunction)xrStringToPath;
    g_clientFunctionMap["xrPathToString"] = (PFN_xrVoidFunction)xrPathToString;
    g_clientFunctionMap["xrGetOpenGLGraphicsRequirementsKHR"] = (PFN_xrVoidFunction)xrGetOpenGLGraphicsRequirementsKHR;
}

// xrGetInstanceProcAddr
XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                     PFN_xrVoidFunction* function) {
    LOG_DEBUG("xrGetInstanceProcAddr called");
    if (!name || !function) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    LOG_DEBUG(("xrGetInstanceProcAddr called for: " + std::string(name)).c_str());

    if (g_clientFunctionMap.empty()) {
        InitializeFunctionMap();
    }

    auto it = g_clientFunctionMap.find(name);
    if (it != g_clientFunctionMap.end()) {
        *function = it->second;
        return XR_SUCCESS;
    }

    LOG_ERROR(("xrGetInstanceProcAddr: Function NOT FOUND: " + std::string(name)).c_str());
    *function = nullptr;
    return XR_ERROR_FUNCTION_UNSUPPORTED;
}

// Negotiation function
extern "C" RUNTIME_EXPORT XRAPI_ATTR XrResult XRAPI_CALL
xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo* loaderInfo, XrNegotiateRuntimeRequest* runtimeRequest) {
    LOG_DEBUG("xrNegotiateLoaderRuntimeInterface called");
    if (!loaderInfo || !runtimeRequest) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo)) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    if (runtimeRequest->structType != XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST ||
        runtimeRequest->structVersion != XR_RUNTIME_INFO_STRUCT_VERSION ||
        runtimeRequest->structSize != sizeof(XrNegotiateRuntimeRequest)) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    runtimeRequest->runtimeInterfaceVersion = XR_CURRENT_LOADER_RUNTIME_VERSION;
    runtimeRequest->runtimeApiVersion = XR_CURRENT_API_VERSION;
    runtimeRequest->getInstanceProcAddr = xrGetInstanceProcAddr;

    return XR_SUCCESS;
}
