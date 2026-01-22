#pragma once

#include <atomic>
#include <mutex>

#include "../protocol/control_channel.h"
#include "../protocol/messages.h"
#include "../protocol/shared_memory.h"

namespace ox {
namespace client {

// Client connection to service
class ServiceConnection {
   public:
    static ServiceConnection& Instance() {
        static ServiceConnection instance;
        return instance;
    }

    bool Connect();
    void Disconnect();
    bool IsConnected() const { return connected_; }

    protocol::SharedData* GetSharedData() { return shared_data_; }
    protocol::ControlChannel& GetControlChannel() { return control_; }

    // Send control message and wait for response
    bool SendRequest(protocol::MessageType type, const void* payload = nullptr, uint32_t payload_size = 0);

    // Allocate a handle from the service
    uint64_t AllocateHandle(protocol::HandleType type);

    // Get next event from service (returns true if event available)
    bool GetNextEvent(protocol::SessionStateEvent& event);

    // Get cached static metadata (queried once on connect)
    const protocol::RuntimePropertiesResponse& GetRuntimeProperties() const { return runtime_props_; }
    const protocol::SystemPropertiesResponse& GetSystemProperties() const { return system_props_; }
    const protocol::ViewConfigurationsResponse& GetViewConfigurations() const { return view_configs_; }
    const protocol::InteractionProfilesResponse& GetInteractionProfiles() const { return interaction_profiles_; }

    // Query input state from driver - type-specific calls
    bool GetInputStateBoolean(const char* user_path, const char* component_path, int64_t predicted_time,
                              uint32_t& out_value, bool& out_available);
    bool GetInputStateFloat(const char* user_path, const char* component_path, int64_t predicted_time, float& out_value,
                            bool& out_available);
    bool GetInputStateVector2f(const char* user_path, const char* component_path, int64_t predicted_time, float& out_x,
                               float& out_y, bool& out_available);

   private:
    ServiceConnection() : shared_data_(nullptr), connected_(false), sequence_(0) {}
    ~ServiceConnection() { Disconnect(); }

    bool QueryStaticMetadata();

    protocol::SharedMemory shared_mem_;
    protocol::ControlChannel control_;
    protocol::SharedData* shared_data_;
    std::atomic<bool> connected_;
    std::atomic<uint32_t> sequence_;
    std::mutex send_mutex_;

    // Cached static metadata (queried once on connect)
    protocol::RuntimePropertiesResponse runtime_props_;
    protocol::SystemPropertiesResponse system_props_;
    protocol::ViewConfigurationsResponse view_configs_;
    protocol::InteractionProfilesResponse interaction_profiles_;
};

}  // namespace client
}  // namespace ox
