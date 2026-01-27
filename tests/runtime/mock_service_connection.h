#pragma once

#include <atomic>
#include <cstring>
#include <mutex>

#include "../../src/client/iservice_connection.h"
#include "../../src/protocol/messages.h"

namespace ox {
namespace test {

/**
 * Mock service connection for testing runtime.cpp in isolation
 *
 * This mock replaces ServiceConnection to allow testing the OpenXR runtime logic
 * without requiring the actual ox-service to be running. It simulates IPC responses
 * that would normally come from the service.
 */
class MockServiceConnection : public client::IServiceConnection {
   public:
    static MockServiceConnection& Instance() {
        static MockServiceConnection instance;
        return instance;
    }

    // IServiceConnection interface implementation
    bool Connect() override;
    void Disconnect() override;
    bool IsConnected() const override { return connected_; }

    protocol::SharedData* GetSharedData() override { return mock_shared_data_; }
    protocol::ControlChannel& GetControlChannel() override {
        // Not used in mock - tests use direct method calls
        static protocol::ControlChannel dummy;
        return dummy;
    }

    // Send control message and wait for response
    bool SendRequest(protocol::MessageType type, const void* payload = nullptr, uint32_t payload_size = 0) override;

    // Allocate a handle from the mock service
    uint64_t AllocateHandle(protocol::HandleType type) override;

    // Get next event from mock service
    bool GetNextEvent(protocol::SessionStateEvent& event) override;

    // Get cached static metadata
    const protocol::RuntimePropertiesResponse& GetRuntimeProperties() const override { return runtime_props_; }
    const protocol::SystemPropertiesResponse& GetSystemProperties() const override { return system_props_; }
    const protocol::ViewConfigurationsResponse& GetViewConfigurations() const override { return view_configs_; }
    const protocol::InteractionProfilesResponse& GetInteractionProfiles() const override {
        return interaction_profiles_;
    }

    // Query input state from mock driver
    bool GetInputStateBoolean(const char* user_path, const char* component_path, int64_t predicted_time,
                              XrBool32& out_value) override;
    bool GetInputStateFloat(const char* user_path, const char* component_path, int64_t predicted_time,
                            float& out_value) override;
    bool GetInputStateVector2f(const char* user_path, const char* component_path, int64_t predicted_time,
                               XrVector2f& out_value) override;

    // Test control methods
    void Reset();
    void SetRuntimeProperties(const char* name, uint32_t major, uint32_t minor, uint32_t patch);
    void SetSystemProperties(const char* name, uint32_t max_width, uint32_t max_height);
    void AddInteractionProfile(const char* profile_path);
    void SetSessionState(protocol::SessionState state);
    void QueueSessionStateEvent(protocol::SessionState state);

   private:
    MockServiceConnection();
    ~MockServiceConnection();

    void InitializeDefaults();

    // Mock data - use pointer to avoid deleted default constructor issue
    protocol::SharedData* mock_shared_data_;
    std::atomic<bool> connected_;
    std::atomic<uint32_t> next_handle_;
    std::mutex mutex_;

    // Cached metadata
    protocol::RuntimePropertiesResponse runtime_props_;
    protocol::SystemPropertiesResponse system_props_;
    protocol::ViewConfigurationsResponse view_configs_;
    protocol::InteractionProfilesResponse interaction_profiles_;

    // Event queue
    std::vector<protocol::SessionStateEvent> event_queue_;
    size_t event_read_index_;
};

}  // namespace test
}  // namespace ox
