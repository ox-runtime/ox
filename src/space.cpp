// Reference space creation and management
#include "common.h"
#include "logging.h"
#include "runtime.h"

// xrCreateReferenceSpace
XRAPI_ATTR XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo,
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
XRAPI_ATTR XrResult XRAPI_CALL xrDestroySpace(XrSpace space) {
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
