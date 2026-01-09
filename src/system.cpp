// System queries and properties
#include <cstring>

#include "common.h"
#include "logging.h"
#include "runtime.h"

// xrGetSystem
XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties(XrInstance instance, XrSystemId systemId,
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
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId,
                                                             uint32_t viewConfigurationTypeCapacityInput,
                                                             uint32_t* viewConfigurationTypeCountOutput,
                                                             XrViewConfigurationType* viewConfigurationTypes) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId,
                                                                 XrViewConfigurationType viewConfigurationType,
                                                                 uint32_t viewCapacityInput, uint32_t* viewCountOutput,
                                                                 XrViewConfigurationView* views) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes(XrInstance instance, XrSystemId systemId,
                                                                XrViewConfigurationType viewConfigurationType,
                                                                uint32_t environmentBlendModeCapacityInput,
                                                                uint32_t* environmentBlendModeCountOutput,
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
