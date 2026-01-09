#pragma once

// Platform headers must be included before OpenXR headers
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <unknwn.h>
#include <windows.h>
#else
// Linux requires X11 and GLX headers
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

// Define graphics API before including OpenXR
#define XR_USE_GRAPHICS_API_OPENGL

// Include necessary headers
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <openxr/openxr_platform.h>

#include <string>
#include <unordered_map>

// Forward declarations for loader negotiation types
struct XrNegotiateLoaderInfo;
struct XrNegotiateRuntimeRequest;

// Runtime information
#define RUNTIME_NAME "ox"
#define RUNTIME_VERSION XR_MAKE_VERSION(1, 0, 0)

// Export macro for Windows DLL
#ifdef _WIN32
#define RUNTIME_EXPORT __declspec(dllexport)
#else
#define RUNTIME_EXPORT __attribute__((visibility("default")))
#endif

// Forward declare all runtime functions
extern "C" {
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(uint32_t propertyCapacityInput,
                                                             uint32_t* propertyCountOutput,
                                                             XrApiLayerProperties* properties);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(const char* layerName,
                                                                      uint32_t propertyCapacityInput,
                                                                      uint32_t* propertyCountOutput,
                                                                      XrExtensionProperties* properties);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(XrInstance instance);

XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties);

XRAPI_ATTR XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData);

XRAPI_ATTR XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value,
                                                char buffer[XR_MAX_RESULT_STRING_SIZE]);

XRAPI_ATTR XrResult XRAPI_CALL xrStructureTypeToString(XrInstance instance, XrStructureType value,
                                                       char buffer[XR_MAX_STRUCTURE_NAME_SIZE]);

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId);

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties(XrInstance instance, XrSystemId systemId,
                                                     XrSystemProperties* properties);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId,
                                                             uint32_t viewConfigurationTypeCapacityInput,
                                                             uint32_t* viewConfigurationTypeCountOutput,
                                                             XrViewConfigurationType* viewConfigurationTypes);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId,
                                                                 XrViewConfigurationType viewConfigurationType,
                                                                 uint32_t viewCapacityInput, uint32_t* viewCountOutput,
                                                                 XrViewConfigurationView* views);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo,
                                               XrSession* session);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroySession(XrSession session);

XRAPI_ATTR XrResult XRAPI_CALL xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrEndSession(XrSession session);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo,
                                                      XrSpace* space);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroySpace(XrSpace space);

XRAPI_ATTR XrResult XRAPI_CALL xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo,
                                           XrFrameState* frameState);

XRAPI_ATTR XrResult XRAPI_CALL xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo,
                                             XrViewState* viewState, uint32_t viewCapacityInput,
                                             uint32_t* viewCountOutput, XrView* views);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes(XrInstance instance, XrSystemId systemId,
                                                                XrViewConfigurationType viewConfigurationType,
                                                                uint32_t environmentBlendModeCapacityInput,
                                                                uint32_t* environmentBlendModeCountOutput,
                                                                XrEnvironmentBlendMode* environmentBlendModes);

XRAPI_ATTR XrResult XRAPI_CALL xrGetOpenGLGraphicsRequirementsKHR(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsOpenGLKHR* graphicsRequirements);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo,
                                                 XrActionSet* actionSet);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet actionSet);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo,
                                              XrAction* action);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyAction(XrAction action);

XRAPI_ATTR XrResult XRAPI_CALL
xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings);

XRAPI_ATTR XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession session,
                                                         const XrSessionActionSetsAttachInfo* attachInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput,
                                                           uint32_t* formatCountOutput, int64_t* formats);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo,
                                                 XrSwapchain* swapchain);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain swapchain);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput,
                                                          uint32_t* imageCountOutput,
                                                          XrSwapchainImageBaseHeader* images);

XRAPI_ATTR XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain swapchain,
                                                       const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index);

XRAPI_ATTR XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain swapchain,
                                                       const XrSwapchainImageReleaseInfo* releaseInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                     PFN_xrVoidFunction* function);

RUNTIME_EXPORT XRAPI_ATTR XrResult XRAPI_CALL
xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo* loaderInfo, XrNegotiateRuntimeRequest* runtimeRequest);
}

// These are shared across all translation units that include this header
inline const std::unordered_map<XrResult, const char*> g_resultStrings = {
    {XR_SUCCESS, "XR_SUCCESS"},
    {XR_ERROR_VALIDATION_FAILURE, "XR_ERROR_VALIDATION_FAILURE"},
    {XR_ERROR_HANDLE_INVALID, "XR_ERROR_HANDLE_INVALID"},
    {XR_ERROR_INSTANCE_LOST, "XR_ERROR_INSTANCE_LOST"},
    {XR_ERROR_RUNTIME_FAILURE, "XR_ERROR_RUNTIME_FAILURE"},
    {XR_EVENT_UNAVAILABLE, "XR_EVENT_UNAVAILABLE"},
};

inline const std::unordered_map<XrStructureType, const char*> g_structureTypeStrings = {
    {XR_TYPE_INSTANCE_CREATE_INFO, "XR_TYPE_INSTANCE_CREATE_INFO"},
    {XR_TYPE_INSTANCE_PROPERTIES, "XR_TYPE_INSTANCE_PROPERTIES"},
};

// Inline map for function dispatch (C++17+)
inline std::unordered_map<std::string, PFN_xrVoidFunction> g_functionMap = {
    {"xrGetInstanceProcAddr", (PFN_xrVoidFunction)xrGetInstanceProcAddr},
    {"xrEnumerateApiLayerProperties", (PFN_xrVoidFunction)xrEnumerateApiLayerProperties},
    {"xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction)xrEnumerateInstanceExtensionProperties},
    {"xrCreateInstance", (PFN_xrVoidFunction)xrCreateInstance},
    {"xrDestroyInstance", (PFN_xrVoidFunction)xrDestroyInstance},
    {"xrGetInstanceProperties", (PFN_xrVoidFunction)xrGetInstanceProperties},
    {"xrPollEvent", (PFN_xrVoidFunction)xrPollEvent},
    {"xrResultToString", (PFN_xrVoidFunction)xrResultToString},
    {"xrStructureTypeToString", (PFN_xrVoidFunction)xrStructureTypeToString},
    {"xrGetSystem", (PFN_xrVoidFunction)xrGetSystem},
    {"xrGetSystemProperties", (PFN_xrVoidFunction)xrGetSystemProperties},
    {"xrEnumerateViewConfigurations", (PFN_xrVoidFunction)xrEnumerateViewConfigurations},
    {"xrEnumerateViewConfigurationViews", (PFN_xrVoidFunction)xrEnumerateViewConfigurationViews},
    {"xrCreateSession", (PFN_xrVoidFunction)xrCreateSession},
    {"xrDestroySession", (PFN_xrVoidFunction)xrDestroySession},
    {"xrBeginSession", (PFN_xrVoidFunction)xrBeginSession},
    {"xrEndSession", (PFN_xrVoidFunction)xrEndSession},
    {"xrCreateReferenceSpace", (PFN_xrVoidFunction)xrCreateReferenceSpace},
    {"xrDestroySpace", (PFN_xrVoidFunction)xrDestroySpace},
    {"xrWaitFrame", (PFN_xrVoidFunction)xrWaitFrame},
    {"xrBeginFrame", (PFN_xrVoidFunction)xrBeginFrame},
    {"xrEndFrame", (PFN_xrVoidFunction)xrEndFrame},
    {"xrLocateViews", (PFN_xrVoidFunction)xrLocateViews},
    {"xrEnumerateEnvironmentBlendModes", (PFN_xrVoidFunction)xrEnumerateEnvironmentBlendModes},
    {"xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction)xrGetOpenGLGraphicsRequirementsKHR},
    {"xrCreateActionSet", (PFN_xrVoidFunction)xrCreateActionSet},
    {"xrDestroyActionSet", (PFN_xrVoidFunction)xrDestroyActionSet},
    {"xrCreateAction", (PFN_xrVoidFunction)xrCreateAction},
    {"xrDestroyAction", (PFN_xrVoidFunction)xrDestroyAction},
    {"xrSuggestInteractionProfileBindings", (PFN_xrVoidFunction)xrSuggestInteractionProfileBindings},
    {"xrAttachSessionActionSets", (PFN_xrVoidFunction)xrAttachSessionActionSets},
    {"xrSyncActions", (PFN_xrVoidFunction)xrSyncActions},
    {"xrEnumerateSwapchainFormats", (PFN_xrVoidFunction)xrEnumerateSwapchainFormats},
    {"xrCreateSwapchain", (PFN_xrVoidFunction)xrCreateSwapchain},
    {"xrDestroySwapchain", (PFN_xrVoidFunction)xrDestroySwapchain},
    {"xrEnumerateSwapchainImages", (PFN_xrVoidFunction)xrEnumerateSwapchainImages},
    {"xrAcquireSwapchainImage", (PFN_xrVoidFunction)xrAcquireSwapchainImage},
    {"xrWaitSwapchainImage", (PFN_xrVoidFunction)xrWaitSwapchainImage},
    {"xrReleaseSwapchainImage", (PFN_xrVoidFunction)xrReleaseSwapchainImage},
};
