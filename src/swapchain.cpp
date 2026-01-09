// Swapchain management
#include "runtime.h"
#include "logging.h"

// xrEnumerateSwapchainFormats
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput,
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
XRAPI_ATTR XrResult XRAPI_CALL xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo,
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
XRAPI_ATTR XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain swapchain) {
    LOG_DEBUG("xrDestroySwapchain called");
    return XR_SUCCESS;
}

// xrEnumerateSwapchainImages
XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput,
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
XRAPI_ATTR XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain swapchain,
                                                       const XrSwapchainImageAcquireInfo* acquireInfo,
                                                       uint32_t* index) {
    LOG_DEBUG("xrAcquireSwapchainImage called");
    if (index) {
        *index = 0;  // Always return the first image
    }
    return XR_SUCCESS;
}

// xrWaitSwapchainImage
XRAPI_ATTR XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) {
    LOG_DEBUG("xrWaitSwapchainImage called");
    return XR_SUCCESS;
}

// xrReleaseSwapchainImage
XRAPI_ATTR XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain swapchain,
                                                       const XrSwapchainImageReleaseInfo* releaseInfo) {
    LOG_DEBUG("xrReleaseSwapchainImage called");
    return XR_SUCCESS;
}
