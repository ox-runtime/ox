// Graphics API requirements
#include "logging.h"
#include "runtime.h"

// xrGetOpenGLGraphicsRequirementsKHR
XRAPI_ATTR XrResult XRAPI_CALL xrGetOpenGLGraphicsRequirementsKHR(
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
