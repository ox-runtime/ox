// Session lifecycle management
#include "common.h"
#include "logging.h"
#include "runtime.h"

// xrCreateSession
XRAPI_ATTR XrResult XRAPI_CALL xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo,
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
XRAPI_ATTR XrResult XRAPI_CALL xrDestroySession(XrSession session) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrEndSession(XrSession session) {
    LOG_DEBUG("xrEndSession called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrEndSession: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    return XR_SUCCESS;
}
