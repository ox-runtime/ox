// Instance lifecycle and properties
#include <cstring>

#include "common.h"
#include "logging.h"
#include "runtime.h"

// xrEnumerateApiLayerProperties
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(uint32_t propertyCapacityInput,
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
        strncpy(properties[i].extensionName, extensions[i], XR_MAX_EXTENSION_NAME_SIZE - 1);
        properties[i].extensionName[XR_MAX_EXTENSION_NAME_SIZE - 1] = '\0';
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
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(XrInstance instance) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) {
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
