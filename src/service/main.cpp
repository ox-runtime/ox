#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <linux/limits.h>
#include <unistd.h>
#endif

#include "../logging.h"
#include "../protocol/control_channel.h"
#include "../protocol/messages.h"
#include "../protocol/shared_memory.h"
#include "driver_loader.h"

using namespace ox::protocol;
namespace fs = std::filesystem;

class OxService {
   public:
    OxService() : running_(false), frame_counter_(0), next_handle_(1) {}

    bool Initialize() {
        LOG_INFO("ox-service: Initializing...");

        // Load headset driver first
        if (!LoadDriver()) {
            LOG_ERROR("Failed to load headset driver");
            return false;
        }

        // Create shared memory (only for dynamic data)
        if (!shared_mem_.Create("ox_runtime_shm", sizeof(SharedData), true)) {
            LOG_ERROR("Failed to create shared memory");
            return false;
        }

        shared_data_ = static_cast<SharedData*>(shared_mem_.GetPointer());

        // Initialize protocol metadata
        shared_data_->protocol_version.store(PROTOCOL_VERSION, std::memory_order_release);
        shared_data_->service_ready.store(1, std::memory_order_release);
        shared_data_->client_connected.store(0, std::memory_order_release);

        // Initialize static properties (stored in service, not shared memory)
        InitializeRuntimeProperties();
        InitializeSystemProperties();
        InitializeViewConfigurations();

        // Initialize session state (dynamic)
        shared_data_->session_state.store(static_cast<uint32_t>(SessionState::IDLE), std::memory_order_release);
        shared_data_->active_session_handle.store(0, std::memory_order_release);

        // Create control channel
        if (!control_.CreateServer("ox_runtime_control")) {
            LOG_ERROR("Failed to create control channel");
            return false;
        }

        LOG_INFO("ox-service: Initialized successfully");
        return true;
    }

    void Run() {
        running_ = true;

        // Start frame generation thread (runs continuously)
        std::thread frame_thread([this]() { FrameLoop(); });

        // Main service loop - accept multiple clients over time
        while (running_) {
            LOG_INFO("ox-service: Waiting for client connection...");

            if (!control_.Accept()) {
                LOG_ERROR("Failed to accept client connection");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            LOG_INFO("ox-service: Client connected");
            shared_data_->client_connected.store(1, std::memory_order_release);

            // Handle control messages for this client
            MessageLoop();

            // Client disconnected, clean up and wait for next client
            shared_data_->client_connected.store(0, std::memory_order_release);
            control_.Close();

            // Recreate control channel for next client
            if (!control_.CreateServer("ox_runtime_control")) {
                LOG_ERROR("Failed to recreate control channel");
                break;
            }

            LOG_INFO("ox-service: Client disconnected, ready for next connection");
        }

        running_ = false;
        if (frame_thread.joinable()) {
            frame_thread.join();
        }

        LOG_INFO("ox-service: Shutting down");
    }

    void Shutdown() {
        running_ = false;
        control_.Close();
        shared_mem_.Close();
        UnlinkSharedMemory("ox_runtime_shm");
    }

   private:
    bool LoadDriver() {
        // Get executable path to find drivers folder
        fs::path exe_path;
#ifdef _WIN32
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        exe_path = fs::path(path);
#else
        char path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) {
            path[len] = '\0';
            exe_path = fs::path(path);
        }
#endif

        fs::path drivers_dir = exe_path.parent_path() / "drivers";

        if (!fs::exists(drivers_dir)) {
            LOG_ERROR(("Drivers folder not found: " + drivers_dir.string()).c_str());
            return false;
        }

        LOG_INFO(("Scanning for drivers in: " + drivers_dir.string()).c_str());

        // Scan for drivers (first-found wins)
        for (const auto& entry : fs::directory_iterator(drivers_dir)) {
            if (!entry.is_directory()) continue;

            fs::path driver_path = entry.path();
            LOG_INFO(("Checking driver: " + driver_path.filename().string()).c_str());

            // Try to load this driver
            if (driver_.LoadDriver(driver_path.string())) {
                // Check if device is connected
                if (driver_.IsDeviceConnected()) {
                    OxDeviceInfo info;
                    driver_.GetDeviceInfo(&info);
                    LOG_INFO(("Loaded driver: " + std::string(info.name)).c_str());
                    return true;
                } else {
                    LOG_INFO("Driver loaded but device not connected");
                    driver_.Unload();
                }
            }
        }

        LOG_ERROR("No connected headset found");
        return false;
    }

    void InitializeRuntimeProperties() {
        std::strncpy(runtime_props_.runtime_name, "ox", sizeof(runtime_props_.runtime_name) - 1);
        runtime_props_.runtime_name[sizeof(runtime_props_.runtime_name) - 1] = '\0';
        runtime_props_.runtime_version_major = 1;
        runtime_props_.runtime_version_minor = 0;
        runtime_props_.runtime_version_patch = 0;
        runtime_props_.padding = 0;
    }

    void InitializeSystemProperties() {
        // Get system name from driver
        OxDeviceInfo device_info;
        driver_.GetDeviceInfo(&device_info);
        std::strncpy(system_props_.system_name, device_info.name, sizeof(system_props_.system_name) - 1);
        system_props_.system_name[sizeof(system_props_.system_name) - 1] = '\0';

        // Get display properties from driver
        OxDisplayProperties display_props;
        driver_.GetDisplayProperties(&display_props);
        system_props_.max_swapchain_width = display_props.display_width;
        system_props_.max_swapchain_height = display_props.display_height;
        system_props_.max_layer_count = 16;

        // Get tracking capabilities from driver
        OxTrackingCapabilities tracking_caps;
        driver_.GetTrackingCapabilities(&tracking_caps);
        system_props_.orientation_tracking = tracking_caps.has_orientation_tracking;
        system_props_.position_tracking = tracking_caps.has_position_tracking;
    }

    void InitializeViewConfigurations() {
        // Get display properties from driver
        OxDisplayProperties display_props;
        driver_.GetDisplayProperties(&display_props);

        // Left and right eye configurations
        for (int i = 0; i < 2; i++) {
            view_configs_.views[i].recommended_width = display_props.recommended_width;
            view_configs_.views[i].recommended_height = display_props.recommended_height;
            view_configs_.views[i].recommended_sample_count = 1;
            view_configs_.views[i].max_sample_count = 4;
        }
    }

    uint64_t AllocateHandle(HandleType type) {
        std::lock_guard<std::mutex> lock(handle_mutex_);
        uint64_t handle = next_handle_++;
        allocated_handles_[handle] = type;
        LOG_DEBUG("Allocated handle for type");
        return handle;
    }

    void FreeHandle(uint64_t handle) {
        std::lock_guard<std::mutex> lock(handle_mutex_);
        allocated_handles_.erase(handle);
    }

    void TransitionSessionState(SessionState new_state) {
        SessionState old_state = static_cast<SessionState>(shared_data_->session_state.load(std::memory_order_acquire));

        if (old_state != new_state) {
            shared_data_->session_state.store(static_cast<uint32_t>(new_state), std::memory_order_release);

            // Queue state change event
            SessionStateEvent event;
            event.session_handle = shared_data_->active_session_handle.load(std::memory_order_acquire);
            event.state = new_state;

            auto now = std::chrono::steady_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
            event.timestamp = ns.count();

            std::lock_guard<std::mutex> lock(event_mutex_);
            pending_events_.push_back(event);

            LOG_INFO("Session state transition");
        }
    }

    void MessageLoop() {
        MessageHeader header;
        std::vector<uint8_t> payload;

        // Loop for current client only (not the global running_ flag)
        while (true) {
            if (!control_.Receive(header, payload)) {
                LOG_ERROR("Control channel receive failed - client likely disconnected");
                return;  // Exit loop for this client
            }

            LOG_DEBUG("Received message type");

            switch (header.type) {
                case MessageType::CONNECT:
                    HandleConnect(header);
                    break;
                case MessageType::DISCONNECT:
                    LOG_INFO("Client requested disconnect");
                    return;  // Exit MessageLoop, service continues running
                case MessageType::CREATE_SESSION:
                    HandleCreateSession(header);
                    break;
                case MessageType::DESTROY_SESSION:
                    HandleDestroySession(header);
                    break;
                case MessageType::ALLOCATE_HANDLE:
                    HandleAllocateHandle(header, payload);
                    break;
                case MessageType::GET_NEXT_EVENT:
                    HandleGetNextEvent(header);
                    break;
                case MessageType::GET_RUNTIME_PROPERTIES:
                    HandleGetRuntimeProperties(header);
                    break;
                case MessageType::GET_SYSTEM_PROPERTIES:
                    HandleGetSystemProperties(header);
                    break;
                case MessageType::GET_VIEW_CONFIGURATIONS:
                    HandleGetViewConfigurations(header);
                    break;
                case MessageType::GET_INTERACTION_PROFILES:
                    HandleGetInteractionProfiles(header);
                    break;
                case MessageType::GET_INPUT_COMPONENT_STATE:
                    HandleGetInputComponentState(header, payload);
                    break;
                default:
                    LOG_ERROR("Unknown message type");
                    break;
            }
        }
    }

    void HandleConnect(const MessageHeader& request) {
        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = 0;
        control_.Send(response);
        LOG_INFO("Sent connect response");
    }

    void HandleCreateSession(const MessageHeader& request) {
        // Allocate session handle
        uint64_t session_handle = AllocateHandle(HandleType::SESSION);
        shared_data_->active_session_handle.store(session_handle, std::memory_order_release);

        // Transition to READY state
        TransitionSessionState(SessionState::READY);

        // After a brief moment, transition to SYNCHRONIZED then FOCUSED
        // (In a real implementation, this would be based on actual hardware state)
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            TransitionSessionState(SessionState::SYNCHRONIZED);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            TransitionSessionState(SessionState::FOCUSED);
        }).detach();

        // Send response with session handle
        AllocateHandleResponse response_payload;
        response_payload.handle = session_handle;

        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(response_payload);
        control_.Send(response, &response_payload);

        LOG_INFO("Session created with handle");
    }

    void HandleDestroySession(const MessageHeader& request) {
        uint64_t session_handle = shared_data_->active_session_handle.load(std::memory_order_acquire);
        if (session_handle != 0) {
            FreeHandle(session_handle);
            shared_data_->active_session_handle.store(0, std::memory_order_release);
        }

        TransitionSessionState(SessionState::IDLE);

        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = 0;
        control_.Send(response);
        LOG_INFO("Session destroyed");
    }

    void HandleAllocateHandle(const MessageHeader& request, const std::vector<uint8_t>& payload) {
        if (payload.size() < sizeof(AllocateHandleRequest)) {
            LOG_ERROR("Invalid allocate handle request");
            return;
        }

        const AllocateHandleRequest* req = reinterpret_cast<const AllocateHandleRequest*>(payload.data());
        uint64_t handle = AllocateHandle(req->handle_type);

        AllocateHandleResponse response_payload;
        response_payload.handle = handle;

        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(response_payload);
        control_.Send(response, &response_payload);
    }

    void HandleGetNextEvent(const MessageHeader& request) {
        std::lock_guard<std::mutex> lock(event_mutex_);

        if (!pending_events_.empty()) {
            SessionStateEvent event = pending_events_.front();
            pending_events_.erase(pending_events_.begin());

            MessageHeader response;
            response.type = MessageType::RESPONSE;
            response.sequence = request.sequence;
            response.payload_size = sizeof(event);
            control_.Send(response, &event);
        } else {
            // No events available
            MessageHeader response;
            response.type = MessageType::RESPONSE;
            response.sequence = request.sequence;
            response.payload_size = 0;
            control_.Send(response);
        }
    }

    void HandleGetRuntimeProperties(const MessageHeader& request) {
        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(runtime_props_);
        control_.Send(response, &runtime_props_);
        LOG_DEBUG("Sent runtime properties");
    }

    void HandleGetSystemProperties(const MessageHeader& request) {
        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(system_props_);
        control_.Send(response, &system_props_);
        LOG_DEBUG("Sent system properties");
    }

    void HandleGetViewConfigurations(const MessageHeader& request) {
        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(view_configs_);
        control_.Send(response, &view_configs_);
        LOG_DEBUG("Sent view configurations");
    }

    void HandleGetInteractionProfiles(const MessageHeader& request) {
        InteractionProfilesResponse profiles_response = {};

        // Get supported profiles from driver
        auto profiles = driver_.GetInteractionProfiles();
        profiles_response.profile_count = static_cast<uint32_t>(profiles.size());

        for (size_t i = 0; i < profiles.size() && i < 8; i++) {
            std::strncpy(profiles_response.profiles[i], profiles[i].c_str(), 127);
            profiles_response.profiles[i][127] = '\0';
        }

        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(profiles_response);
        control_.Send(response, &profiles_response);
        LOG_DEBUG("Sent interaction profiles");
    }

    void HandleGetInputComponentState(const MessageHeader& request, const std::vector<uint8_t>& payload) {
        if (payload.size() < sizeof(InputComponentStateRequest)) {
            LOG_ERROR("Invalid input component state request");
            return;
        }

        const InputComponentStateRequest* component_request =
            reinterpret_cast<const InputComponentStateRequest*>(payload.data());

        InputComponentStateResponse component_response = {};
        component_response.is_available = driver_.GetInputComponentState(
            component_request->predicted_time, component_request->controller_index, component_request->component_path,
            &component_response.boolean_value, &component_response.float_value, &component_response.x,
            &component_response.y);

        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = sizeof(component_response);
        control_.Send(response, &component_response);
        LOG_DEBUG("Sent input component state");
    }

    void FrameLoop() {
        LOG_INFO("Frame generation thread started");

        using namespace std::chrono;
        auto next_frame = steady_clock::now();
        const auto frame_interval = duration_cast<steady_clock::duration>(duration<double>(1.0 / 90.0));

        while (running_) {
            next_frame += frame_interval;

            // Generate tracking data from driver
            UpdatePoseData();

            // Sleep until next frame
            std::this_thread::sleep_until(next_frame);
        }

        LOG_INFO("Frame generation thread stopped");
    }

    void UpdatePoseData() {
        auto& frame = shared_data_->frame_state;

        // Update frame counter
        uint64_t frame_id = frame_counter_++;
        frame.frame_id.store(frame_id, std::memory_order_release);

        // Get timestamp (nanoseconds)
        auto now = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
        int64_t predicted_time = ns.count();
        frame.predicted_display_time.store(predicted_time, std::memory_order_release);

        // Get poses from driver
        frame.view_count.store(2, std::memory_order_release);

        OxPose view_pose;

        // Left eye
        driver_.UpdateViewPose(predicted_time, 0, &view_pose);
        frame.views[0].pose.position[0] = view_pose.position.x;
        frame.views[0].pose.position[1] = view_pose.position.y;
        frame.views[0].pose.position[2] = view_pose.position.z;
        frame.views[0].pose.orientation[0] = view_pose.orientation.x;
        frame.views[0].pose.orientation[1] = view_pose.orientation.y;
        frame.views[0].pose.orientation[2] = view_pose.orientation.z;
        frame.views[0].pose.orientation[3] = view_pose.orientation.w;
        frame.views[0].pose.timestamp = predicted_time;

        // Right eye
        driver_.UpdateViewPose(predicted_time, 1, &view_pose);
        frame.views[1].pose.position[0] = view_pose.position.x;
        frame.views[1].pose.position[1] = view_pose.position.y;
        frame.views[1].pose.position[2] = view_pose.position.z;
        frame.views[1].pose.orientation[0] = view_pose.orientation.x;
        frame.views[1].pose.orientation[1] = view_pose.orientation.y;
        frame.views[1].pose.orientation[2] = view_pose.orientation.z;
        frame.views[1].pose.orientation[3] = view_pose.orientation.w;
        frame.views[1].pose.timestamp = predicted_time;

        // Get FOV from driver
        OxDisplayProperties display_props;
        driver_.GetDisplayProperties(&display_props);

        for (int i = 0; i < 2; i++) {
            frame.views[i].fov[0] = display_props.fov.angle_left;
            frame.views[i].fov[1] = display_props.fov.angle_right;
            frame.views[i].fov[2] = display_props.fov.angle_up;
            frame.views[i].fov[3] = display_props.fov.angle_down;
        }

        // Update controller poses (left and right)
        for (int i = 0; i < 2; i++) {
            OxControllerState controller_state;
            driver_.UpdateControllerState(predicted_time, i, &controller_state);

            frame.controller_poses[i].position[0] = controller_state.pose.position.x;
            frame.controller_poses[i].position[1] = controller_state.pose.position.y;
            frame.controller_poses[i].position[2] = controller_state.pose.position.z;
            frame.controller_poses[i].orientation[0] = controller_state.pose.orientation.x;
            frame.controller_poses[i].orientation[1] = controller_state.pose.orientation.y;
            frame.controller_poses[i].orientation[2] = controller_state.pose.orientation.z;
            frame.controller_poses[i].orientation[3] = controller_state.pose.orientation.w;
            frame.controller_poses[i].timestamp = predicted_time;
            frame.controller_poses[i].flags.store(controller_state.is_active, std::memory_order_release);
        }
    }

    SharedMemory shared_mem_;
    ControlChannel control_;
    SharedData* shared_data_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> frame_counter_;

    // Driver
    ox::driver::DriverLoader driver_;

    // Static properties (queried via control channel, not in shared memory)
    RuntimePropertiesResponse runtime_props_;
    SystemPropertiesResponse system_props_;
    ViewConfigurationsResponse view_configs_;

    // Handle allocation
    std::atomic<uint64_t> next_handle_;
    std::mutex handle_mutex_;
    std::unordered_map<uint64_t, HandleType> allocated_handles_;

    std::mutex event_mutex_;
    std::vector<SessionStateEvent> pending_events_;
};

int main(int argc, char** argv) {
    LOG_INFO("=== ox-service starting ===");

    OxService service;

    if (!service.Initialize()) {
        LOG_ERROR("Failed to initialize service");
        return 1;
    }

    service.Run();
    service.Shutdown();

    LOG_INFO("=== ox-service stopped ===");
    return 0;
}
