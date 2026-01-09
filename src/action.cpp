// Actions and interaction profiles
#include "runtime.h"
#include "logging.h"

// xrCreateActionSet
XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo,
                                                 XrActionSet* actionSet) {
    LOG_DEBUG("xrCreateActionSet called");
    if (!createInfo || !actionSet) {
        LOG_ERROR("xrCreateActionSet: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    *actionSet = (XrActionSet)1;  // Return a dummy action set
    return XR_SUCCESS;
}

// xrDestroyActionSet
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet actionSet) {
    LOG_DEBUG("xrDestroyActionSet called");
    return XR_SUCCESS;
}

// xrCreateAction
XRAPI_ATTR XrResult XRAPI_CALL xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo,
                                              XrAction* action) {
    LOG_DEBUG("xrCreateAction called");
    if (!createInfo || !action) {
        LOG_ERROR("xrCreateAction: Invalid parameters");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    *action = (XrAction)1;  // Return a dummy action
    return XR_SUCCESS;
}

// xrDestroyAction
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyAction(XrAction action) {
    LOG_DEBUG("xrDestroyAction called");
    return XR_SUCCESS;
}

// xrSuggestInteractionProfileBindings
XRAPI_ATTR XrResult XRAPI_CALL xrSuggestInteractionProfileBindings(
    XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    LOG_DEBUG("xrSuggestInteractionProfileBindings called");
    return XR_SUCCESS;
}

// xrAttachSessionActionSets
XRAPI_ATTR XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession session,
                                                         const XrSessionActionSetsAttachInfo* attachInfo) {
    LOG_DEBUG("xrAttachSessionActionSets called");
    return XR_SUCCESS;
}

// xrSyncActions
XRAPI_ATTR XrResult XRAPI_CALL xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
    LOG_DEBUG("xrSyncActions called");
    return XR_SUCCESS;
}
