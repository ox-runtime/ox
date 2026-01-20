#pragma once

#include <atomic>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#endif

namespace ox {
namespace protocol {

// Protocol version
static constexpr uint32_t PROTOCOL_VERSION = 2;

// Message types for control channel
enum class MessageType : uint32_t {
    CONNECT = 1,
    DISCONNECT = 2,
    CREATE_SESSION = 3,
    DESTROY_SESSION = 4,
    BEGIN_FRAME = 5,
    END_FRAME = 6,
    SHARE_GRAPHICS_HANDLE = 7,
    ALLOCATE_HANDLE = 8,
    GET_NEXT_EVENT = 9,
    GET_RUNTIME_PROPERTIES = 10,
    GET_SYSTEM_PROPERTIES = 11,
    GET_VIEW_CONFIGURATIONS = 12,
    GET_INTERACTION_PROFILES = 13,
    GET_INPUT_COMPONENT_STATE = 14,
    RESPONSE = 100,
};

// OpenXR handle types
enum class HandleType : uint32_t {
    INSTANCE = 1,
    SESSION = 2,
    SPACE = 3,
    ACTION_SET = 4,
    ACTION = 5,
    SWAPCHAIN = 6,
};

// Session states
enum class SessionState : uint32_t {
    UNKNOWN = 0,
    IDLE = 1,
    READY = 2,
    SYNCHRONIZED = 3,
    VISIBLE = 4,
    FOCUSED = 5,
    STOPPING = 6,
    EXITING = 7,
};

// Status codes for protocol messages
enum MessageStatus : uint32_t {
    OK = 0,
    FAILED = 1,
    NOT_READY = 2,
};

// Control message header (fixed size for simple protocol)
struct MessageHeader {
    MessageType type;
    uint32_t sequence;
    uint32_t payload_size;
    uint32_t reserved;
};

// Message payloads
struct AllocateHandleRequest {
    HandleType handle_type;
};

struct AllocateHandleResponse {
    uint64_t handle;
};

struct SessionStateEvent {
    uint64_t session_handle;
    SessionState state;
    uint64_t timestamp;
};

// Response payloads for static metadata queries (via control channel)
struct RuntimePropertiesResponse {
    char runtime_name[128];
    uint32_t runtime_version_major;
    uint32_t runtime_version_minor;
    uint32_t runtime_version_patch;
    uint32_t padding;
};

struct SystemPropertiesResponse {
    char system_name[256];
    uint32_t max_swapchain_width;
    uint32_t max_swapchain_height;
    uint32_t max_layer_count;
    uint32_t orientation_tracking;
    uint32_t position_tracking;
    uint32_t padding[2];
};

struct ViewConfigurationsResponse {
    struct ViewConfig {
        uint32_t recommended_width;
        uint32_t recommended_height;
        uint32_t recommended_sample_count;
        uint32_t max_sample_count;
    } views[2];  // Stereo
};

struct InteractionProfilesResponse {
    uint32_t profile_count;
    char profiles[8][128];  // Up to 8 interaction profile paths
};

struct InputComponentStateRequest {
    char user_path[256];       // e.g., "/user/hand/left", "/user/vive_tracker_htcx/role/waist"
    char component_path[128];  // e.g., "/input/trigger/value"
    int64_t predicted_time;
};

struct InputComponentStateResponse {
    uint32_t is_available;  // 0 = unavailable, 1 = available
    uint32_t boolean_value;
    float float_value;
    float x;
    float y;
};

// Pose data (hot path - shared memory)
struct alignas(64) Pose {
    float position[3];
    float orientation[4];  // Quaternion (x, y, z, w)
    uint64_t timestamp;
    std::atomic<uint32_t> flags;
    uint32_t padding;
};

// Device pose data (controllers, trackers, etc.)
#define MAX_TRACKED_DEVICES 16  // Support up to 16 devices

struct DevicePose {
    char user_path[256];  // OpenXR user path
    Pose pose;
    uint32_t is_active;
    uint32_t padding;
};

// View data
struct View {
    Pose pose;
    float fov[4];  // angleLeft, angleRight, angleUp, angleDown
};

// Frame state (shared memory hot path)
struct alignas(64) FrameState {
    std::atomic<uint64_t> frame_id;
    std::atomic<uint64_t> predicted_display_time;
    std::atomic<uint32_t> view_count;
    std::atomic<uint32_t> flags;

    View views[2];  // Support stereo for now

    // Tracked devices (controllers, trackers, etc.)
    std::atomic<uint32_t> device_count;
    uint32_t padding1;
    DevicePose device_poses[MAX_TRACKED_DEVICES];

    // Graphics handles (platform specific)
#ifdef _WIN32
    uint64_t texture_handles[2];  // HANDLE as uint64_t
#else
    int32_t texture_fds[2];
#endif
};

// Shared memory layout (only dynamic data - hot path optimized)
constexpr std::size_t SHARED_DATA_SIZE = 8192;  // 8KB total size

// Shared data with automatic sizing
struct alignas(4096) SharedData {
    union {
        struct {
            std::atomic<uint32_t> protocol_version;
            std::atomic<uint32_t> service_ready;
            std::atomic<uint32_t> client_connected;
            uint32_t padding1;

            // Session state (dynamic)
            std::atomic<uint32_t> session_state;  // SessionState enum
            std::atomic<uint64_t> active_session_handle;

            // Frame state (hot path - 90Hz updates)
            FrameState frame_state;
        } fields;

        // Union with raw array ensures exact size without manual padding calculation
        std::byte raw[SHARED_DATA_SIZE];
    };
};

static_assert(sizeof(SharedData) == SHARED_DATA_SIZE, "SharedData size doesn't match SHARED_DATA_SIZE");
static_assert(alignof(SharedData) == 4096, "SharedData alignment is not 4096 bytes");

}  // namespace protocol
}  // namespace ox
