// Event polling
#include "logging.h"
#include "runtime.h"

// xrPollEvent - returns session state change events
XRAPI_ATTR XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
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
