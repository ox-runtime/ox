// ToString functions for debugging
#include <cstring>

#include "logging.h"
#include "runtime.h"

// xrResultToString
XRAPI_ATTR XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value,
                                                char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    LOG_DEBUG("xrResultToString called");
    if (!buffer) {
        LOG_ERROR("xrResultToString: Null buffer");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    auto it = g_resultStrings.find(value);
    const char* resultStr = (it != g_resultStrings.end()) ? it->second : "XR_UNKNOWN";

    strncpy(buffer, resultStr, XR_MAX_RESULT_STRING_SIZE - 1);
    buffer[XR_MAX_RESULT_STRING_SIZE - 1] = '\0';

    return XR_SUCCESS;
}

// xrStructureTypeToString
XRAPI_ATTR XrResult XRAPI_CALL xrStructureTypeToString(XrInstance instance, XrStructureType value,
                                                       char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    LOG_DEBUG("xrStructureTypeToString called");
    if (!buffer) {
        LOG_ERROR("xrStructureTypeToString: Null buffer");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    auto it = g_structureTypeStrings.find(value);
    const char* typeStr = (it != g_structureTypeStrings.end()) ? it->second : "XR_TYPE_UNKNOWN";

    strncpy(buffer, typeStr, XR_MAX_STRUCTURE_NAME_SIZE - 1);
    buffer[XR_MAX_STRUCTURE_NAME_SIZE - 1] = '\0';

    return XR_SUCCESS;
}
