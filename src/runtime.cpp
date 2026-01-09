// Include platform headers before OpenXR
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

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
static std::mutex g_instance_mutex;
static uint64_t g_next_handle = 1;

// Helper to generate unique handles
XrInstance createHandle() { return (XrInstance)(uintptr_t)(g_next_handle++); }

// Function to get the function pointer for OpenXR API functions
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                                PFN_xrVoidFunction* function);

// xrEnumerateApiLayerProperties
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(uint32_t propertyCapacityInput,
                                                                        uint32_t* propertyCountOutput,
                                                                        XrApiLayerProperties* properties) {
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
    // No extensions in this minimal runtime
    if (propertyCountOutput) {
        *propertyCountOutput = 0;
    }
    return XR_SUCCESS;
}

// xrCreateInstance
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo* createInfo,
                                                           XrInstance* instance) {
    if (!createInfo || !instance) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Create a new instance handle
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    XrInstance newInstance = createHandle();
    g_instances[newInstance] = true;
    *instance = newInstance;

    return XR_SUCCESS;
}

// xrDestroyInstance
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(XrInstance instance) {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    g_instances.erase(it);
    return XR_SUCCESS;
}

// xrGetInstanceProperties
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance,
                                                                  XrInstanceProperties* instanceProperties) {
    if (!instanceProperties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Check if instance is valid
    {
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        if (g_instances.find(instance) == g_instances.end()) {
            return XR_ERROR_HANDLE_INVALID;
        }
    }

    // Fill in runtime properties
    instanceProperties->runtimeVersion = RUNTIME_VERSION;
    strncpy(instanceProperties->runtimeName, RUNTIME_NAME, XR_MAX_RUNTIME_NAME_SIZE - 1);
    instanceProperties->runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = '\0';

    return XR_SUCCESS;
}

// xrPollEvent - stub implementation
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
    // No events in minimal runtime
    return XR_EVENT_UNAVAILABLE;
}

// xrResultToString
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value,
                                                           char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    if (!buffer) {
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
    if (!buffer) {
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

// xrGetInstanceProcAddr - the main function dispatch table
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name,
                                                                PFN_xrVoidFunction* function) {
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
