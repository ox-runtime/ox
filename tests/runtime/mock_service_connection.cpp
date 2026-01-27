#include "mock_service_connection.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "../../src/logging.h"

namespace ox {
namespace test {

MockServiceConnection::MockServiceConnection()
    : connected_(false), next_handle_(1000), event_read_index_(0), mock_shared_data_(nullptr) {
    // Allocate aligned memory for SharedData
    mock_shared_data_ =
        static_cast<protocol::SharedData*>(::operator new(sizeof(protocol::SharedData), std::align_val_t(4096)));
    InitializeDefaults();
}

MockServiceConnection::~MockServiceConnection() {
    if (mock_shared_data_) {
        ::operator delete(mock_shared_data_, std::align_val_t(4096));
        mock_shared_data_ = nullptr;
    }
}

void MockServiceConnection::InitializeDefaults() {
    // Initialize shared data (using memset is safe for this union type)
    std::memset(mock_shared_data_, 0, sizeof(protocol::SharedData));
    mock_shared_data_->fields.protocol_version.store(protocol::PROTOCOL_VERSION, std::memory_order_release);
    mock_shared_data_->fields.service_ready.store(1, std::memory_order_release);
    mock_shared_data_->fields.client_connected.store(0, std::memory_order_release);
    mock_shared_data_->fields.session_state.store(static_cast<uint32_t>(protocol::SessionState::IDLE),
                                                  std::memory_order_release);

    // Initialize runtime properties
    std::memset(&runtime_props_, 0, sizeof(runtime_props_));
    std::strncpy(runtime_props_.runtime_name, "ox Runtime (Mock)", sizeof(runtime_props_.runtime_name) - 1);
    runtime_props_.runtime_version_major = 0;
    runtime_props_.runtime_version_minor = 1;
    runtime_props_.runtime_version_patch = 0;

    // Initialize system properties
    std::memset(&system_props_, 0, sizeof(system_props_));
    std::strncpy(system_props_.system_name, "Mock VR System", sizeof(system_props_.system_name) - 1);
    system_props_.max_swapchain_width = 2048;
    system_props_.max_swapchain_height = 2048;
    system_props_.max_layer_count = 16;
    system_props_.orientation_tracking = 1;
    system_props_.position_tracking = 1;

    // Initialize view configurations (stereo)
    std::memset(&view_configs_, 0, sizeof(view_configs_));
    view_configs_.views[0].recommended_width = 1832;
    view_configs_.views[0].recommended_height = 1920;
    view_configs_.views[0].recommended_sample_count = 1;
    view_configs_.views[0].max_sample_count = 4;
    view_configs_.views[1].recommended_width = 1832;
    view_configs_.views[1].recommended_height = 1920;
    view_configs_.views[1].recommended_sample_count = 1;
    view_configs_.views[1].max_sample_count = 4;

    // Initialize interaction profiles
    std::memset(&interaction_profiles_, 0, sizeof(interaction_profiles_));
    interaction_profiles_.profile_count = 0;

    // Clear event queue
    event_queue_.clear();
    event_read_index_ = 0;
}

bool MockServiceConnection::Connect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (connected_) {
        return true;
    }

    LOG_INFO("Mock: Connecting to mock service");
    mock_shared_data_->fields.client_connected.store(1, std::memory_order_release);
    connected_ = true;

    return true;
}

void MockServiceConnection::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return;
    }

    LOG_INFO("Mock: Disconnecting from mock service");
    mock_shared_data_->fields.client_connected.store(0, std::memory_order_release);
    connected_ = false;
}

bool MockServiceConnection::SendRequest(protocol::MessageType type, const void* payload, uint32_t payload_size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return false;
    }

    // Mock just returns success for simple requests
    LOG_DEBUG("Mock: SendRequest");
    return true;
}

uint64_t MockServiceConnection::AllocateHandle(protocol::HandleType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return 0;
    }

    uint64_t handle = next_handle_++;
    LOG_DEBUG("Mock: Allocated handle");
    return handle;
}

bool MockServiceConnection::GetNextEvent(protocol::SessionStateEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return false;
    }

    // Return queued events if available
    if (event_read_index_ < event_queue_.size()) {
        event = event_queue_[event_read_index_++];
        return true;
    }

    return false;
}

bool MockServiceConnection::GetInputStateBoolean(const char* user_path, const char* component_path,
                                                 int64_t predicted_time, XrBool32& out_value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return false;
    }

    // Mock: Return false for all boolean inputs by default
    out_value = XR_FALSE;
    return true;
}

bool MockServiceConnection::GetInputStateFloat(const char* user_path, const char* component_path,
                                               int64_t predicted_time, float& out_value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return false;
    }

    // Mock: Return 0.0 for all float inputs by default
    out_value = 0.0f;
    return true;
}

bool MockServiceConnection::GetInputStateVector2f(const char* user_path, const char* component_path,
                                                  int64_t predicted_time, XrVector2f& out_value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connected_) {
        return false;
    }

    // Mock: Return (0, 0) for all vector2f inputs by default
    out_value.x = 0.0f;
    out_value.y = 0.0f;
    return true;
}

void MockServiceConnection::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    connected_ = false;
    next_handle_ = 1000;
    event_queue_.clear();
    event_read_index_ = 0;
    InitializeDefaults();
}

void MockServiceConnection::SetRuntimeProperties(const char* name, uint32_t major, uint32_t minor, uint32_t patch) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::strncpy(runtime_props_.runtime_name, name, sizeof(runtime_props_.runtime_name) - 1);
    runtime_props_.runtime_name[sizeof(runtime_props_.runtime_name) - 1] = '\0';
    runtime_props_.runtime_version_major = major;
    runtime_props_.runtime_version_minor = minor;
    runtime_props_.runtime_version_patch = patch;
}

void MockServiceConnection::SetSystemProperties(const char* name, uint32_t max_width, uint32_t max_height) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::strncpy(system_props_.system_name, name, sizeof(system_props_.system_name) - 1);
    system_props_.system_name[sizeof(system_props_.system_name) - 1] = '\0';
    system_props_.max_swapchain_width = max_width;
    system_props_.max_swapchain_height = max_height;
}

void MockServiceConnection::AddInteractionProfile(const char* profile_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (interaction_profiles_.profile_count >= 8) {
        LOG_ERROR("Mock: Cannot add more than 8 interaction profiles");
        return;
    }

    std::strncpy(interaction_profiles_.profiles[interaction_profiles_.profile_count], profile_path, 127);
    interaction_profiles_.profiles[interaction_profiles_.profile_count][127] = '\0';
    interaction_profiles_.profile_count++;
}

void MockServiceConnection::SetSessionState(protocol::SessionState state) {
    std::lock_guard<std::mutex> lock(mutex_);

    mock_shared_data_->fields.session_state.store(static_cast<uint32_t>(state), std::memory_order_release);
}

void MockServiceConnection::QueueSessionStateEvent(protocol::SessionState state) {
    std::lock_guard<std::mutex> lock(mutex_);

    protocol::SessionStateEvent event;
    event.session_handle = 0;  // Mock session handle
    event.state = state;
    event.timestamp = 0;

    event_queue_.push_back(event);
}

}  // namespace test
}  // namespace ox
