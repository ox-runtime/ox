#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OX_DRIVER_API_VERSION 1

// Platform-specific export macro
#ifdef _WIN32
#define OX_DRIVER_EXPORT __declspec(dllexport)
#else
#define OX_DRIVER_EXPORT __attribute__((visibility("default")))
#endif

// Forward declarations
typedef struct OxDriverCallbacks OxDriverCallbacks;

// 3D position vector
typedef struct {
    float x, y, z;
} OxVector3f;

// Quaternion for orientation
typedef struct {
    float x, y, z, w;
} OxQuaternion;

// 6DOF pose (position + orientation)
typedef struct {
    OxVector3f position;
    OxQuaternion orientation;
} OxPose;

// Field of view (radians)
typedef struct {
    float angle_left;
    float angle_right;
    float angle_up;
    float angle_down;
} OxFov;

// Device information
typedef struct {
    char name[256];          // e.g., "Dummy VR Headset"
    char manufacturer[256];  // e.g., "ox runtime"
    char serial[256];        // e.g., "DUMMY-12345"
    uint32_t vendor_id;
    uint32_t product_id;
} OxDeviceInfo;

// Display capabilities
typedef struct {
    uint32_t display_width;       // Per-eye width in pixels
    uint32_t display_height;      // Per-eye height in pixels
    uint32_t recommended_width;   // Recommended render target width
    uint32_t recommended_height;  // Recommended render target height
    float refresh_rate;           // Hz
    OxFov fov;                    // Field of view
} OxDisplayProperties;

// Tracking capabilities
typedef struct {
    uint32_t has_position_tracking;
    uint32_t has_orientation_tracking;
} OxTrackingCapabilities;

// Device information (controllers, trackers, etc.)
#define OX_MAX_DEVICES 16

typedef struct {
    char user_path[256];  // OpenXR user path: "/user/hand/left", "/user/vive_tracker_htcx/role/waist", etc.
    OxPose pose;
    uint32_t is_active;  // 1 if device is connected/tracked, 0 otherwise
} OxDeviceState;

// Component state result codes
typedef enum {
    OX_COMPONENT_UNAVAILABLE = 0,  // Component doesn't exist on this controller
    OX_COMPONENT_AVAILABLE = 1,    // Component exists and state is valid
} OxComponentResult;

// Input component state - generic for all component types
typedef struct {
    // Boolean state (for /click, /touch components)
    uint32_t boolean_value;  // 0 or 1

    // Float state (for /value, /force components)
    float float_value;  // 0.0 to 1.0

    // Vector2 state (for thumbstick/trackpad x/y)
    float x;  // -1.0 to 1.0
    float y;  // -1.0 to 1.0
} OxInputComponentState;

// Driver callbacks - implement these in your driver
struct OxDriverCallbacks {
    // ========== Lifecycle ==========

    // Called once when driver is loaded
    // Return: 1 on success, 0 on failure
    int (*initialize)(void);

    // Called when runtime shuts down
    void (*shutdown)(void);

    // ========== Device Discovery ==========

    // Check if physical device is connected and ready
    // Return: 1 if connected, 0 if not
    int (*is_device_connected)(void);

    // Get device information (name, manufacturer, serial, etc.)
    void (*get_device_info)(OxDeviceInfo* info);

    // ========== Display Properties ==========

    // Get display specifications
    void (*get_display_properties)(OxDisplayProperties* props);

    // Get tracking capabilities
    void (*get_tracking_capabilities)(OxTrackingCapabilities* caps);

    // ========== Hot Path - Called Every Frame ==========

    // Update HMD pose for given predicted display time
    // predicted_time: nanoseconds since epoch
    // out_pose: write the HMD pose here
    void (*update_pose)(int64_t predicted_time, OxPose* out_pose);

    // Update per-eye view poses (typically just IPD offset from HMD pose)
    // predicted_time: nanoseconds since epoch
    // eye_index: 0 = left, 1 = right
    // out_pose: write the eye pose here
    void (*update_view_pose)(int64_t predicted_time, uint32_t eye_index, OxPose* out_pose);

    // ========== Devices (Controllers, Trackers, etc.) ==========

    // Update all tracked devices (controllers, trackers, etc.)
    // predicted_time: nanoseconds since epoch
    // out_states: array to fill with device states (must have space for OX_MAX_DEVICES)
    // out_count: write the number of devices here (must be <= OX_MAX_DEVICES)
    // This callback is optional - set to NULL if no tracked devices are supported
    void (*update_devices)(int64_t predicted_time, OxDeviceState* out_states, uint32_t* out_count);

    // Get input component state for a device
    // predicted_time: nanoseconds since epoch
    // user_path: OpenXR user path (e.g., "/user/hand/left", "/user/vive_tracker_htcx/role/waist")
    // component_path: OpenXR component path (e.g., "/input/trigger/value", "/input/a/click", "/input/thumbstick")
    // out_state: write the component state here
    // Returns: OX_COMPONENT_AVAILABLE if component exists, OX_COMPONENT_UNAVAILABLE otherwise
    //
    // Example component paths:
    //   "/input/trigger/value"      -> float_value (0.0 to 1.0)
    //   "/input/trigger/click"      -> boolean_value (0 or 1)
    //   "/input/a/click"            -> boolean_value
    //   "/input/a/touch"            -> boolean_value
    //   "/input/squeeze/value"      -> float_value
    //   "/input/thumbstick"         -> x, y (-1.0 to 1.0)
    //   "/input/thumbstick/x"       -> float_value (-1.0 to 1.0)
    //   "/input/thumbstick/click"   -> boolean_value
    //
    // This callback is optional - set to NULL if devices are not supported
    OxComponentResult (*get_input_component_state)(int64_t predicted_time, const char* user_path,
                                                   const char* component_path, OxInputComponentState* out_state);

    // ========== Interaction Profiles (Optional) ==========

    // Get supported interaction profiles for controllers
    // profiles: array to fill with null-terminated profile path strings
    // max_profiles: size of the profiles array
    // Returns: number of supported profiles (may be > max_profiles)
    // Example profile: "/interaction_profiles/khr/simple_controller"
    // This callback is optional - if NULL, driver supports /interaction_profiles/khr/simple_controller by default
    uint32_t (*get_interaction_profiles)(const char** profiles, uint32_t max_profiles);
};

// Every driver MUST export this function
// The runtime calls this to register the driver's callbacks
// callbacks: pointer to struct that runtime has allocated
// Return: 1 on success, 0 on failure
typedef int (*OxDriverRegisterFunc)(OxDriverCallbacks* callbacks);

#ifdef __cplusplus
}
#endif
