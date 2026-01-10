#pragma once

#include <atomic>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#endif

// Platform-specific alignment macros
#ifdef _MSC_VER
#define ALIGN_64 __declspec(align(64))
#define ALIGN_4096 __declspec(align(4096))
#else
#define ALIGN_64 __attribute__((aligned(64)))
#define ALIGN_4096 __attribute__((aligned(4096)))
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

// Pose data (hot path - shared memory)
struct ALIGN_64 Pose {
    float position[3];
    float orientation[4];  // Quaternion (x, y, z, w)
    uint64_t timestamp;
    uint32_t flags;
    uint32_t padding;
};

// View data
struct View {
    Pose pose;
    float fov[4];  // angleLeft, angleRight, angleUp, angleDown
};

// Frame state (shared memory hot path)
struct ALIGN_64 FrameState {
    std::atomic<uint64_t> frame_id;
    std::atomic<uint64_t> predicted_display_time;
    std::atomic<uint32_t> view_count;
    std::atomic<uint32_t> flags;

    View views[2];  // Support stereo for now

    // Graphics handles (platform specific)
#ifdef _WIN32
    uint64_t texture_handles[2];  // HANDLE as uint64_t
#else
    int32_t texture_fds[2];
#endif
};

// Shared memory layout (only dynamic data - hot path optimized)
struct ALIGN_4096 SharedData {
    std::atomic<uint32_t> protocol_version;
    std::atomic<uint32_t> service_ready;
    std::atomic<uint32_t> client_connected;
    uint32_t padding1;

    // Session state (dynamic)
    std::atomic<uint32_t> session_state;  // SessionState enum
    std::atomic<uint64_t> active_session_handle;

    // Frame state (hot path - 90Hz updates)
    FrameState frame_state;

    uint8_t reserved[3568];  // Pad to 4KB
};

static_assert(sizeof(SharedData) <= 4096, "SharedData must fit in 4KB page");

}  // namespace protocol
}  // namespace ox
