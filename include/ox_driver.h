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
};

// Every driver MUST export this function
// The runtime calls this to register the driver's callbacks
// callbacks: pointer to struct that runtime has allocated
// Return: 1 on success, 0 on failure
typedef int (*OxDriverRegisterFunc)(OxDriverCallbacks* callbacks);

#ifdef __cplusplus
}
#endif
