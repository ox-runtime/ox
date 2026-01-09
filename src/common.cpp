// Shared state and utilities
#include <mutex>
#include <unordered_map>

#include "logging.h"
#include "runtime.h"

// Instance handle management
std::unordered_map<XrInstance, bool> g_instances;
std::unordered_map<XrSession, XrInstance> g_sessions;
std::unordered_map<XrSpace, XrSession> g_spaces;
std::mutex g_instance_mutex;
uint64_t g_next_handle = 1;
uint64_t g_frame_counter = 0;

// Session state tracking
bool g_sessionReadySent = false;
bool g_sessionFocusedSent = false;
