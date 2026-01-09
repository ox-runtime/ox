// Include platform headers before OpenXR
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <unknwn.h>
#include <windows.h>
#else
// Linux requires X11 and GLX headers
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <openxr/openxr_platform.h>

#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

#include "logging.h"

// Export macro for Windows DLL
#ifdef _WIN32
#define RUNTIME_EXPORT __declspec(dllexport)
#else
#define RUNTIME_EXPORT __attribute__((visibility("default")))
#endif

// Runtime information
#define RUNTIME_NAME "ox"
#define RUNTIME_VERSION XR_MAKE_VERSION(1, 0, 0)

// Instance handle management
static std::unordered_map<XrInstance, bool> g_instances;
static std::unordered_map<XrSession, XrInstance> g_sessions;
static std::unordered_map<XrSpace, XrSession> g_spaces;
static std::mutex g_instance_mutex;
static uint64_t g_next_handle = 1;
static uint64_t g_frame_counter = 0;

// Helper to generate unique handles
template <typename T>
T createHandle() {
    return (T)(uintptr_t)(g_next_handle++);
}

// Function to get the function pointer for OpenXR API functions
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                                PFN_xrVoidFunction* function);

// xrEnumerateApiLayerProperties
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(uint32_t propertyCapacityInput,
                                                                        uint32_t* propertyCountOutput,
                                                                        XrApiLayerProperties* properties) {
    LOG_DEBUG("xrEnumerateApiLayerProperties called");
    // No API layers in this minimal runtime
    if (propertyCountOutput) {
        *propertyCountOutput = 0;
    }
    return XR_SUCCESS;
}

// xrEnumerateInstanceExtensionProperties
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(const char* layerName,
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
        strncpy(properties[i].extensionName, extensions[i], XR_MAX_EXTENSION_NAME_SIZE - 1);
        properties[i].extensionName[XR_MAX_EXTENSION_NAME_SIZE - 1] = '\0';
    }

    return XR_SUCCESS;
}

// xrCreateInstance
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo* createInfo,
                                                           XrInstance* instance) {
    LOG_DEBUG("xrCreateInstance called");
    if (!createInfo || !instance) {
        LOG_ERROR("xrCreateInstance: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO) {
        LOG_ERROR("xrCreateInstance: Invalid structure type");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Create a new instance handle
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    XrInstance newInstance = createHandle<XrInstance>();
    g_instances[newInstance] = true;
    *instance = newInstance;

    LOG_INFO("OpenXR instance created successfully");
    return XR_SUCCESS;
}

// xrDestroyInstance
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(XrInstance instance) {
    LOG_DEBUG("xrDestroyInstance called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        LOG_ERROR("xrDestroyInstance: Invalid instance handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    g_instances.erase(it);
    LOG_INFO("OpenXR instance destroyed");
    return XR_SUCCESS;
}

// xrGetInstanceProperties
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance,
                                                                  XrInstanceProperties* instanceProperties) {
    LOG_DEBUG("xrGetInstanceProperties called");
    if (!instanceProperties) {
        LOG_ERROR("xrGetInstanceProperties: Null instanceProperties");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
        LOG_ERROR("xrGetInstanceProperties: Invalid structure type");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Check if instance is valid
    {
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        if (g_instances.find(instance) == g_instances.end()) {
            LOG_ERROR("xrGetInstanceProperties: Invalid instance handle");
            return XR_ERROR_HANDLE_INVALID;
        }
    }

    // Fill in runtime properties
    instanceProperties->runtimeVersion = RUNTIME_VERSION;
    strncpy(instanceProperties->runtimeName, RUNTIME_NAME, XR_MAX_RUNTIME_NAME_SIZE - 1);
    instanceProperties->runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = '\0';

    return XR_SUCCESS;
}

// Session state tracking
static bool g_sessionReadySent = false;
static bool g_sessionFocusedSent = false;

// xrPollEvent - returns session state change events
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
    LOG_DEBUG("xrPollEvent called");
    if (!eventData) {
        LOG_ERROR("xrPollEvent: Null eventData");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    static bool readySent = false;
    static bool synchronizedSent = false;
    static bool focusedSent = false;

    // Send session state events in sequence
    if (!readySent) {
        XrEventDataSessionStateChanged* stateEvent = (XrEventDataSessionStateChanged*)eventData;
        stateEvent->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
        stateEvent->next = nullptr;
        stateEvent->session = (XrSession)1;
        stateEvent->state = XR_SESSION_STATE_READY;
        stateEvent->time = 0;
        readySent = true;
        LOG_INFO("Session state: READY");
        return XR_SUCCESS;
    }

    if (!synchronizedSent) {
        XrEventDataSessionStateChanged* stateEvent = (XrEventDataSessionStateChanged*)eventData;
        stateEvent->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
        stateEvent->next = nullptr;
        stateEvent->session = (XrSession)1;
        stateEvent->state = XR_SESSION_STATE_SYNCHRONIZED;
        stateEvent->time = 0;
        synchronizedSent = true;
        LOG_INFO("Session state: SYNCHRONIZED");
        return XR_SUCCESS;
    }

    if (!focusedSent) {
        XrEventDataSessionStateChanged* stateEvent = (XrEventDataSessionStateChanged*)eventData;
        stateEvent->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
        stateEvent->next = nullptr;
        stateEvent->session = (XrSession)1;
        stateEvent->state = XR_SESSION_STATE_FOCUSED;
        stateEvent->time = 0;
        focusedSent = true;
        LOG_INFO("Session state: FOCUSED");
        return XR_SUCCESS;
    }

    // No more events
    return XR_EVENT_UNAVAILABLE;
}

// xrResultToString
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value,
                                                           char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    LOG_DEBUG("xrResultToString called");
    if (!buffer) {
        LOG_ERROR("xrResultToString: Null buffer");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    const char* resultStr = "XR_UNKNOWN";
    switch (value) {
        case XR_SUCCESS:
            resultStr = "XR_SUCCESS";
            break;
        case XR_ERROR_VALIDATION_FAILURE:
            resultStr = "XR_ERROR_VALIDATION_FAILURE";
            break;
        case XR_ERROR_HANDLE_INVALID:
            resultStr = "XR_ERROR_HANDLE_INVALID";
            break;
        case XR_ERROR_INSTANCE_LOST:
            resultStr = "XR_ERROR_INSTANCE_LOST";
            break;
        case XR_ERROR_RUNTIME_FAILURE:
            resultStr = "XR_ERROR_RUNTIME_FAILURE";
            break;
        case XR_EVENT_UNAVAILABLE:
            resultStr = "XR_EVENT_UNAVAILABLE";
            break;
        default:
            break;
    }

    strncpy(buffer, resultStr, XR_MAX_RESULT_STRING_SIZE - 1);
    buffer[XR_MAX_RESULT_STRING_SIZE - 1] = '\0';

    return XR_SUCCESS;
}

// xrStructureTypeToString
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrStructureTypeToString(XrInstance instance, XrStructureType value,
                                                                  char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    LOG_DEBUG("xrStructureTypeToString called");
    if (!buffer) {
        LOG_ERROR("xrStructureTypeToString: Null buffer");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    const char* typeStr = "XR_TYPE_UNKNOWN";
    switch (value) {
        case XR_TYPE_INSTANCE_CREATE_INFO:
            typeStr = "XR_TYPE_INSTANCE_CREATE_INFO";
            break;
        case XR_TYPE_INSTANCE_PROPERTIES:
            typeStr = "XR_TYPE_INSTANCE_PROPERTIES";
            break;
        default:
            break;
    }

    strncpy(buffer, typeStr, XR_MAX_STRUCTURE_NAME_SIZE - 1);
    buffer[XR_MAX_STRUCTURE_NAME_SIZE - 1] = '\0';

    return XR_SUCCESS;
}

// xrGetSystem
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo,
                                                      XrSystemId* systemId) {
    LOG_DEBUG("xrGetSystem called");
    if (!getInfo || !systemId) {
        LOG_ERROR("xrGetSystem: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_instances.find(instance) == g_instances.end()) {
        LOG_ERROR("xrGetSystem: Invalid instance handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    // Return a fixed system ID
    *systemId = 1;
    return XR_SUCCESS;
}

// xrGetSystemProperties
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties(XrInstance instance, XrSystemId systemId,
                                                                XrSystemProperties* properties) {
    LOG_DEBUG("xrGetSystemProperties called");
    if (!properties || systemId != 1) {
        LOG_ERROR("xrGetSystemProperties: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    properties->systemId = systemId;
    strncpy(properties->systemName, "ox HMD", XR_MAX_SYSTEM_NAME_SIZE - 1);
    properties->systemName[XR_MAX_SYSTEM_NAME_SIZE - 1] = '\0';
    properties->vendorId = 0x1234;
    properties->graphicsProperties.maxSwapchainImageHeight = 1080;
    properties->graphicsProperties.maxSwapchainImageWidth = 1920;
    properties->graphicsProperties.maxLayerCount = 1;
    properties->trackingProperties.orientationTracking = XR_TRUE;
    properties->trackingProperties.positionTracking = XR_TRUE;

    return XR_SUCCESS;
}

// xrEnumerateViewConfigurations
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurations(
    XrInstance instance, XrSystemId systemId, uint32_t viewConfigurationTypeCapacityInput,
    uint32_t* viewConfigurationTypeCountOutput, XrViewConfigurationType* viewConfigurationTypes) {
    LOG_DEBUG("xrEnumerateViewConfigurations called");
    if (!viewConfigurationTypeCountOutput) {
        LOG_ERROR("xrEnumerateViewConfigurations: Null output parameter");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    *viewConfigurationTypeCountOutput = 1;

    if (viewConfigurationTypeCapacityInput == 0) {
        return XR_SUCCESS;
    }

    if (viewConfigurationTypes) {
        viewConfigurationTypes[0] = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    }

    return XR_SUCCESS;
}

// xrEnumerateViewConfigurationViews
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurationViews(
    XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t viewCapacityInput,
    uint32_t* viewCountOutput, XrViewConfigurationView* views) {
    LOG_DEBUG("xrEnumerateViewConfigurationViews called");
    if (!viewCountOutput) {
        LOG_ERROR("xrEnumerateViewConfigurationViews: Null output parameter");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    *viewCountOutput = 2;  // Stereo

    if (viewCapacityInput == 0) {
        return XR_SUCCESS;
    }

    if (views) {
        for (uint32_t i = 0; i < 2 && i < viewCapacityInput; i++) {
            views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
            views[i].next = nullptr;
            views[i].recommendedImageRectWidth = 960;
            views[i].maxImageRectWidth = 1920;
            views[i].recommendedImageRectHeight = 1080;
            views[i].maxImageRectHeight = 2160;
            views[i].recommendedSwapchainSampleCount = 1;
            views[i].maxSwapchainSampleCount = 1;
        }
    }

    return XR_SUCCESS;
}

// xrEnumerateEnvironmentBlendModes
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes(
    XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType,
    uint32_t environmentBlendModeCapacityInput, uint32_t* environmentBlendModeCountOutput,
    XrEnvironmentBlendMode* environmentBlendModes) {
    LOG_DEBUG("xrEnumerateEnvironmentBlendModes called");
    if (!environmentBlendModeCountOutput) {
        LOG_ERROR("xrEnumerateEnvironmentBlendModes: Null output parameter");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    *environmentBlendModeCountOutput = 1;

    if (environmentBlendModeCapacityInput == 0) {
        return XR_SUCCESS;
    }

    if (environmentBlendModes) {
        environmentBlendModes[0] = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    }

    return XR_SUCCESS;
}

// xrCreateSession
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo,
                                                          XrSession* session) {
    LOG_DEBUG("xrCreateSession called");
    if (!createInfo || !session) {
        LOG_ERROR("xrCreateSession: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_instances.find(instance) == g_instances.end()) {
        LOG_ERROR("xrCreateSession: Invalid instance handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    XrSession newSession = createHandle<XrSession>();
    g_sessions[newSession] = instance;
    *session = newSession;

    LOG_INFO("OpenXR session created successfully");
    return XR_SUCCESS;
}

// xrDestroySession
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroySession(XrSession session) {
    LOG_DEBUG("xrDestroySession called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_sessions.find(session);
    if (it == g_sessions.end()) {
        LOG_ERROR("xrDestroySession: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    g_sessions.erase(it);
    LOG_INFO("OpenXR session destroyed");
    return XR_SUCCESS;
}

// xrBeginSession
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
    LOG_DEBUG("xrBeginSession called");
    if (!beginInfo) {
        LOG_ERROR("xrBeginSession: Null beginInfo");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrBeginSession: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    return XR_SUCCESS;
}

// xrEndSession
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEndSession(XrSession session) {
    LOG_DEBUG("xrEndSession called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrEndSession: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    return XR_SUCCESS;
}

// xrCreateReferenceSpace
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession session,
                                                                 const XrReferenceSpaceCreateInfo* createInfo,
                                                                 XrSpace* space) {
    LOG_DEBUG("xrCreateReferenceSpace called");
    if (!createInfo || !space) {
        LOG_ERROR("xrCreateReferenceSpace: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrCreateReferenceSpace: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    XrSpace newSpace = createHandle<XrSpace>();
    g_spaces[newSpace] = session;
    *space = newSpace;

    return XR_SUCCESS;
}

// xrDestroySpace
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroySpace(XrSpace space) {
    LOG_DEBUG("xrDestroySpace called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_spaces.find(space);
    if (it == g_spaces.end()) {
        LOG_ERROR("xrDestroySpace: Invalid space handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    g_spaces.erase(it);
    return XR_SUCCESS;
}

// xrWaitFrame
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo,
                                                      XrFrameState* frameState) {
    LOG_DEBUG("xrWaitFrame called");
    if (!frameState) {
        LOG_ERROR("xrWaitFrame: Null frameState");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrWaitFrame: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    frameState->type = XR_TYPE_FRAME_STATE;
    frameState->next = nullptr;
    frameState->predictedDisplayTime = ++g_frame_counter * 16666667;  // ~60 FPS in nanoseconds
    frameState->predictedDisplayPeriod = 16666667;
    frameState->shouldRender = XR_TRUE;

    return XR_SUCCESS;
}

// xrBeginFrame
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
    LOG_DEBUG("xrBeginFrame called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrBeginFrame: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    return XR_SUCCESS;
}

// xrEndFrame
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
    LOG_DEBUG("xrEndFrame called");
    if (!frameEndInfo) {
        LOG_ERROR("xrEndFrame: Null frameEndInfo");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrEndFrame: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    return XR_SUCCESS;
}

// xrLocateViews - Returns incrementing position values per frame
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo,
                                                        XrViewState* viewState, uint32_t viewCapacityInput,
                                                        uint32_t* viewCountOutput, XrView* views) {
    LOG_DEBUG("xrLocateViews called");
    if (!viewLocateInfo || !viewState || !viewCountOutput) {
        LOG_ERROR("xrLocateViews: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrLocateViews: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    *viewCountOutput = 2;  // Stereo

    if (viewCapacityInput == 0) {
        return XR_SUCCESS;
    }

    // Set view state flags
    viewState->type = XR_TYPE_VIEW_STATE;
    viewState->next = nullptr;
    viewState->viewStateFlags = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;

    if (views && viewCapacityInput >= 2) {
        // Incrementing position based on frame counter
        float posX = static_cast<float>(g_frame_counter) * 0.01f;  // Increment by 0.01 per frame
        float posY = static_cast<float>(g_frame_counter) * 0.01f;
        float posZ = static_cast<float>(g_frame_counter) * 0.01f;

        for (uint32_t i = 0; i < 2; i++) {
            views[i].type = XR_TYPE_VIEW;
            views[i].next = nullptr;

            // Position increments with each frame
            views[i].pose.position.x = posX + (i == 0 ? -0.032f : 0.032f);  // Eye separation
            views[i].pose.position.y = posY;
            views[i].pose.position.z = posZ;

            // Identity orientation (no rotation)
            views[i].pose.orientation.x = 0.0f;
            views[i].pose.orientation.y = 0.0f;
            views[i].pose.orientation.z = 0.0f;
            views[i].pose.orientation.w = 1.0f;

            // Field of view
            views[i].fov.angleLeft = -0.785f;  // ~45 degrees
            views[i].fov.angleRight = 0.785f;
            views[i].fov.angleUp = 0.785f;
            views[i].fov.angleDown = -0.785f;
        }
    }

    return XR_SUCCESS;
}

// xrCreateActionSet
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSet(XrInstance instance,
                                                            const XrActionSetCreateInfo* createInfo,
                                                            XrActionSet* actionSet) {
    LOG_DEBUG("xrCreateActionSet called");
    if (!createInfo || !actionSet) {
        LOG_ERROR("xrCreateActionSet: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    *actionSet = (XrActionSet)1;  // Return a dummy action set
    return XR_SUCCESS;
}

// xrDestroyActionSet
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet actionSet) {
    LOG_DEBUG("xrDestroyActionSet called");
    return XR_SUCCESS;
}

// xrCreateAction
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo,
                                                         XrAction* action) {
    LOG_DEBUG("xrCreateAction called");
    if (!createInfo || !action) {
        LOG_ERROR("xrCreateAction: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    *action = (XrAction)1;  // Return a dummy action
    return XR_SUCCESS;
}

// xrDestroyAction
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroyAction(XrAction action) {
    LOG_DEBUG("xrDestroyAction called");
    return XR_SUCCESS;
}

// xrSuggestInteractionProfileBindings
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrSuggestInteractionProfileBindings(
    XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    LOG_DEBUG("xrSuggestInteractionProfileBindings called");
    return XR_SUCCESS;
}

// xrAttachSessionActionSets
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession session,
                                                                    const XrSessionActionSetsAttachInfo* attachInfo) {
    LOG_DEBUG("xrAttachSessionActionSets called");
    return XR_SUCCESS;
}

// xrSyncActions
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
    LOG_DEBUG("xrSyncActions called");
    return XR_SUCCESS;
}

// xrEnumerateSwapchainFormats
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput,
                                                                      uint32_t* formatCountOutput, int64_t* formats) {
    LOG_DEBUG("xrEnumerateSwapchainFormats called");
    const int64_t supportedFormats[] = {
        0x1908,  // GL_RGBA
        0x8058,  // GL_RGBA8
    };
    const uint32_t numFormats = 2;

    if (formatCountOutput) {
        *formatCountOutput = numFormats;
    }

    if (formatCapacityInput > 0 && formats) {
        uint32_t count = (formatCapacityInput < numFormats) ? formatCapacityInput : numFormats;
        for (uint32_t i = 0; i < count; i++) {
            formats[i] = supportedFormats[i];
        }
    }

    return XR_SUCCESS;
}

// xrCreateSwapchain
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo,
                                                            XrSwapchain* swapchain) {
    LOG_DEBUG("xrCreateSwapchain called");
    if (!swapchain) {
        LOG_ERROR("xrCreateSwapchain: Null swapchain pointer");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    *swapchain = (XrSwapchain)1;  // Return a dummy swapchain
    return XR_SUCCESS;
}

// xrDestroySwapchain
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain swapchain) {
    LOG_DEBUG("xrDestroySwapchain called");
    return XR_SUCCESS;
}

// xrEnumerateSwapchainImages
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput,
                                                                     uint32_t* imageCountOutput,
                                                                     XrSwapchainImageBaseHeader* images) {
    LOG_DEBUG("xrEnumerateSwapchainImages called");
    const uint32_t numImages = 3;  // Typical triple-buffering

    if (imageCountOutput) {
        *imageCountOutput = numImages;
    }

    if (imageCapacityInput > 0 && images) {
        // For OpenGL, the images would be XrSwapchainImageOpenGLKHR with texture IDs
        // For now, just return success - the application will handle the dummy data
        for (uint32_t i = 0; i < imageCapacityInput && i < numImages; i++) {
            images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
            images[i].next = nullptr;
        }
    }

    return XR_SUCCESS;
}

// xrAcquireSwapchainImage
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain swapchain,
                                                                  const XrSwapchainImageAcquireInfo* acquireInfo,
                                                                  uint32_t* index) {
    LOG_DEBUG("xrAcquireSwapchainImage called");
    if (index) {
        *index = 0;  // Always return the first image
    }
    return XR_SUCCESS;
}

// xrWaitSwapchainImage
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain swapchain,
                                                               const XrSwapchainImageWaitInfo* waitInfo) {
    LOG_DEBUG("xrWaitSwapchainImage called");
    return XR_SUCCESS;
}

// xrReleaseSwapchainImage
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain swapchain,
                                                                  const XrSwapchainImageReleaseInfo* releaseInfo) {
    LOG_DEBUG("xrReleaseSwapchainImage called");
    return XR_SUCCESS;
}

// xrGetOpenGLGraphicsRequirementsKHR
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetOpenGLGraphicsRequirementsKHR(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
    LOG_DEBUG("xrGetOpenGLGraphicsRequirementsKHR called");
    if (!graphicsRequirements) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    // Set minimal OpenGL version requirements
    graphicsRequirements->minApiVersionSupported = XR_MAKE_VERSION(3, 3, 0);
    graphicsRequirements->maxApiVersionSupported = XR_MAKE_VERSION(4, 6, 0);
    return XR_SUCCESS;
}

// xrGetInstanceProcAddr - the main function dispatch table
// xrGetInstanceProcAddr - the main function dispatch table
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                                PFN_xrVoidFunction* function) {
    LOG_DEBUG("xrGetInstanceProcAddr called");
    if (!name || !function) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Map function names to function pointers
    if (strcmp(name, "xrGetInstanceProcAddr") == 0) {
        *function = (PFN_xrVoidFunction)xrGetInstanceProcAddr;
    } else if (strcmp(name, "xrEnumerateApiLayerProperties") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateApiLayerProperties;
    } else if (strcmp(name, "xrEnumerateInstanceExtensionProperties") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateInstanceExtensionProperties;
    } else if (strcmp(name, "xrCreateInstance") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateInstance;
    } else if (strcmp(name, "xrDestroyInstance") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyInstance;
    } else if (strcmp(name, "xrGetInstanceProperties") == 0) {
        *function = (PFN_xrVoidFunction)xrGetInstanceProperties;
    } else if (strcmp(name, "xrPollEvent") == 0) {
        *function = (PFN_xrVoidFunction)xrPollEvent;
    } else if (strcmp(name, "xrResultToString") == 0) {
        *function = (PFN_xrVoidFunction)xrResultToString;
    } else if (strcmp(name, "xrStructureTypeToString") == 0) {
        *function = (PFN_xrVoidFunction)xrStructureTypeToString;
    } else if (strcmp(name, "xrGetSystem") == 0) {
        *function = (PFN_xrVoidFunction)xrGetSystem;
    } else if (strcmp(name, "xrGetSystemProperties") == 0) {
        *function = (PFN_xrVoidFunction)xrGetSystemProperties;
    } else if (strcmp(name, "xrEnumerateViewConfigurations") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateViewConfigurations;
    } else if (strcmp(name, "xrEnumerateViewConfigurationViews") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateViewConfigurationViews;
    } else if (strcmp(name, "xrCreateSession") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateSession;
    } else if (strcmp(name, "xrDestroySession") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroySession;
    } else if (strcmp(name, "xrBeginSession") == 0) {
        *function = (PFN_xrVoidFunction)xrBeginSession;
    } else if (strcmp(name, "xrEndSession") == 0) {
        *function = (PFN_xrVoidFunction)xrEndSession;
    } else if (strcmp(name, "xrCreateReferenceSpace") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateReferenceSpace;
    } else if (strcmp(name, "xrDestroySpace") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroySpace;
    } else if (strcmp(name, "xrWaitFrame") == 0) {
        *function = (PFN_xrVoidFunction)xrWaitFrame;
    } else if (strcmp(name, "xrBeginFrame") == 0) {
        *function = (PFN_xrVoidFunction)xrBeginFrame;
    } else if (strcmp(name, "xrEndFrame") == 0) {
        *function = (PFN_xrVoidFunction)xrEndFrame;
    } else if (strcmp(name, "xrLocateViews") == 0) {
        *function = (PFN_xrVoidFunction)xrLocateViews;
    } else if (strcmp(name, "xrEnumerateEnvironmentBlendModes") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateEnvironmentBlendModes;
    } else if (strcmp(name, "xrGetOpenGLGraphicsRequirementsKHR") == 0) {
        *function = (PFN_xrVoidFunction)xrGetOpenGLGraphicsRequirementsKHR;
    } else if (strcmp(name, "xrCreateActionSet") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateActionSet;
    } else if (strcmp(name, "xrDestroyActionSet") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyActionSet;
    } else if (strcmp(name, "xrCreateAction") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateAction;
    } else if (strcmp(name, "xrDestroyAction") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyAction;
    } else if (strcmp(name, "xrSuggestInteractionProfileBindings") == 0) {
        *function = (PFN_xrVoidFunction)xrSuggestInteractionProfileBindings;
    } else if (strcmp(name, "xrAttachSessionActionSets") == 0) {
        *function = (PFN_xrVoidFunction)xrAttachSessionActionSets;
    } else if (strcmp(name, "xrSyncActions") == 0) {
        *function = (PFN_xrVoidFunction)xrSyncActions;
    } else if (strcmp(name, "xrEnumerateSwapchainFormats") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateSwapchainFormats;
    } else if (strcmp(name, "xrCreateSwapchain") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateSwapchain;
    } else if (strcmp(name, "xrDestroySwapchain") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroySwapchain;
    } else if (strcmp(name, "xrEnumerateSwapchainImages") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateSwapchainImages;
    } else if (strcmp(name, "xrAcquireSwapchainImage") == 0) {
        *function = (PFN_xrVoidFunction)xrAcquireSwapchainImage;
    } else if (strcmp(name, "xrWaitSwapchainImage") == 0) {
        *function = (PFN_xrVoidFunction)xrWaitSwapchainImage;
    } else if (strcmp(name, "xrReleaseSwapchainImage") == 0) {
        *function = (PFN_xrVoidFunction)xrReleaseSwapchainImage;
    } else {
        // Function not supported
        *function = nullptr;
        return XR_ERROR_FUNCTION_UNSUPPORTED;
    }

    return XR_SUCCESS;
}

// Negotiation function required by OpenXR loader
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

    // Set the runtime's interface version
    runtimeRequest->runtimeInterfaceVersion = XR_CURRENT_LOADER_RUNTIME_VERSION;
    runtimeRequest->runtimeApiVersion = XR_CURRENT_API_VERSION;
    runtimeRequest->getInstanceProcAddr = xrGetInstanceProcAddr;

    return XR_SUCCESS;
}

