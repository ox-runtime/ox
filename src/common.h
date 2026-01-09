#pragma once

#include <openxr/openxr.h>

#include <mutex>
#include <unordered_map>

// Shared state across all runtime modules
extern std::unordered_map<XrInstance, bool> g_instances;
extern std::unordered_map<XrSession, XrInstance> g_sessions;
extern std::unordered_map<XrSpace, XrSession> g_spaces;
extern std::mutex g_instance_mutex;
extern uint64_t g_next_handle;
extern uint64_t g_frame_counter;

// Session state tracking
extern bool g_sessionReadySent;
extern bool g_sessionFocusedSent;

// Helper to generate unique handles
template <typename T>
T createHandle() {
    return (T)(uintptr_t)(g_next_handle++);
}
