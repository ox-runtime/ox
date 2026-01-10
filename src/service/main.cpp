#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "../logging.h"
#include "../protocol/control_channel.h"
#include "../protocol/messages.h"
#include "../protocol/shared_memory.h"

using namespace ox::protocol;

class OxService {
   public:
    OxService() : running_(false), frame_counter_(0) {}

    bool Initialize() {
        LOG_INFO("ox-service: Initializing...");

        // Create shared memory
        if (!shared_mem_.Create("ox_runtime_shm", sizeof(SharedData), true)) {
            LOG_ERROR("Failed to create shared memory");
            return false;
        }

        shared_data_ = static_cast<SharedData*>(shared_mem_.GetPointer());
        shared_data_->protocol_version.store(PROTOCOL_VERSION, std::memory_order_release);
        shared_data_->service_ready.store(1, std::memory_order_release);
        shared_data_->client_connected.store(0, std::memory_order_release);

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
        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = 0;
        control_.Send(response);
        LOG_INFO("Session created");
    }

    void HandleDestroySession(const MessageHeader& request) {
        MessageHeader response;
        response.type = MessageType::RESPONSE;
        response.sequence = request.sequence;
        response.payload_size = 0;
        control_.Send(response);
        LOG_INFO("Session destroyed");
    }

    void FrameLoop() {
        LOG_INFO("Frame generation thread started");

        using namespace std::chrono;
        auto next_frame = steady_clock::now();
        const auto frame_interval = duration_cast<steady_clock::duration>(duration<double>(1.0 / 90.0));

        while (running_) {
            next_frame += frame_interval;

            // Generate mock tracking data
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

        // Mock timestamp (nanoseconds)
        auto now = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
        frame.predicted_display_time.store(ns.count(), std::memory_order_release);

        // Mock poses for stereo views
        frame.view_count.store(2, std::memory_order_release);

        // Left eye
        frame.views[0].pose.position[0] = -0.032f;  // IPD/2
        frame.views[0].pose.position[1] = 1.6f;     // Eye height
        frame.views[0].pose.position[2] = 0.0f;
        frame.views[0].pose.orientation[0] = 0.0f;
        frame.views[0].pose.orientation[1] = 0.0f;
        frame.views[0].pose.orientation[2] = 0.0f;
        frame.views[0].pose.orientation[3] = 1.0f;
        frame.views[0].pose.timestamp = ns.count();

        // Right eye
        frame.views[1].pose.position[0] = 0.032f;  // IPD/2
        frame.views[1].pose.position[1] = 1.6f;
        frame.views[1].pose.position[2] = 0.0f;
        frame.views[1].pose.orientation[0] = 0.0f;
        frame.views[1].pose.orientation[1] = 0.0f;
        frame.views[1].pose.orientation[2] = 0.0f;
        frame.views[1].pose.orientation[3] = 1.0f;
        frame.views[1].pose.timestamp = ns.count();

        // Mock FOV
        for (int i = 0; i < 2; i++) {
            frame.views[i].fov[0] = -0.785f;  // ~45 deg left
            frame.views[i].fov[1] = 0.785f;   // ~45 deg right
            frame.views[i].fov[2] = 0.785f;   // ~45 deg up
            frame.views[i].fov[3] = -0.785f;  // ~45 deg down
        }
    }

    SharedMemory shared_mem_;
    ControlChannel control_;
    SharedData* shared_data_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> frame_counter_;
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
