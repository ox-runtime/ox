// Frame timing and view location
#include "common.h"
#include "logging.h"
#include "runtime.h"

// xrWaitFrame
XRAPI_ATTR XrResult XRAPI_CALL xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo,
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
XRAPI_ATTR XrResult XRAPI_CALL xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
    LOG_DEBUG("xrBeginFrame called");
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (g_sessions.find(session) == g_sessions.end()) {
        LOG_ERROR("xrBeginFrame: Invalid session handle");
        return XR_ERROR_HANDLE_INVALID;
    }

    return XR_SUCCESS;
}

// xrEndFrame
XRAPI_ATTR XrResult XRAPI_CALL xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
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
XRAPI_ATTR XrResult XRAPI_CALL xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo,
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
