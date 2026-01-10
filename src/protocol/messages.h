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
static constexpr uint32_t PROTOCOL_VERSION = 1;

// Message types for control channel
enum class MessageType : uint32_t {
    CONNECT = 1,
    DISCONNECT = 2,
    CREATE_SESSION = 3,
    DESTROY_SESSION = 4,
    BEGIN_FRAME = 5,
    END_FRAME = 6,
    SHARE_GRAPHICS_HANDLE = 7,
    RESPONSE = 100,
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

// Shared memory layout
struct ALIGN_4096 SharedData {
    std::atomic<uint32_t> protocol_version;
    std::atomic<uint32_t> service_ready;
    std::atomic<uint32_t> client_connected;
    uint32_t padding1;

    FrameState frame_state;

    uint8_t reserved[3584];  // Pad to 4KB
};

static_assert(sizeof(SharedData) <= 4096, "SharedData must fit in 4KB page");

}  // namespace protocol
}  // namespace ox
