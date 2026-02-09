#pragma once

#include <openxr/openxr.h>

#include <atomic>
#include <mutex>

#include "../protocol/control_channel.h"
#include "../protocol/messages.h"
#include "../protocol/shared_memory.h"

namespace ox {
namespace client {

/**
 * Interface for service connection - allows dependency injection for testing
 *
 * This interface abstracts the ServiceConnection to enable mocking in tests.
 * The runtime uses this interface instead of directly calling ServiceConnection,
 * allowing tests to inject a mock implementation.
 */
class IServiceConnection {
   public:
    virtual ~IServiceConnection() = default;

    // Connection management
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    // Data access
    virtual protocol::SharedData* GetSharedData() = 0;
    virtual protocol::ControlChannel& GetControlChannel() = 0;

    // Control messages
    virtual bool SendRequest(protocol::MessageType type, const void* payload = nullptr, uint32_t payload_size = 0) = 0;
    virtual uint64_t AllocateHandle(protocol::HandleType type) = 0;
    virtual bool GetNextEvent(protocol::SessionStateEvent& event) = 0;

    // Static metadata
    virtual const protocol::RuntimePropertiesResponse& GetRuntimeProperties() const = 0;
    virtual const protocol::SystemPropertiesResponse& GetSystemProperties() const = 0;
    virtual const protocol::ViewConfigurationsResponse& GetViewConfigurations() const = 0;
    virtual const protocol::InteractionProfilesResponse& GetInteractionProfiles() const = 0;

    // Input state queries
    virtual bool GetInputStateBoolean(const char* user_path, const char* component_path, int64_t predicted_time,
                                      XrBool32& out_value) = 0;
    virtual bool GetInputStateFloat(const char* user_path, const char* component_path, int64_t predicted_time,
                                    float& out_value) = 0;
    virtual bool GetInputStateVector2f(const char* user_path, const char* component_path, int64_t predicted_time,
                                       XrVector2f& out_value) = 0;
};

// Client connection to service
class ServiceConnection : public IServiceConnection {
   public:
    static ServiceConnection& Instance() {
        static ServiceConnection instance;
        return instance;
    }

    // IServiceConnection interface implementation
    bool Connect() override;
    void Disconnect() override;
    bool IsConnected() const override { return connected_; }

    protocol::SharedData* GetSharedData() override { return shared_data_; }
    protocol::ControlChannel& GetControlChannel() override { return control_; }

    // Send control message and wait for response
    bool SendRequest(protocol::MessageType type, const void* payload = nullptr, uint32_t payload_size = 0) override;

    // Allocate a handle from the service
    uint64_t AllocateHandle(protocol::HandleType type) override;

    // Get next event from service (returns true if event available)
    bool GetNextEvent(protocol::SessionStateEvent& event) override;

    // Get cached static metadata (queried once on connect)
    const protocol::RuntimePropertiesResponse& GetRuntimeProperties() const override { return runtime_props_; }
    const protocol::SystemPropertiesResponse& GetSystemProperties() const override { return system_props_; }
    const protocol::ViewConfigurationsResponse& GetViewConfigurations() const override { return view_configs_; }
    const protocol::InteractionProfilesResponse& GetInteractionProfiles() const override {
        return interaction_profiles_;
    }

    // Query input state from driver - type-specific calls
    bool GetInputStateBoolean(const char* user_path, const char* component_path, int64_t predicted_time,
                              XrBool32& out_value) override;
    bool GetInputStateFloat(const char* user_path, const char* component_path, int64_t predicted_time,
                            float& out_value) override;
    bool GetInputStateVector2f(const char* user_path, const char* component_path, int64_t predicted_time,
                               XrVector2f& out_value) override;

   private:
    ServiceConnection() : shared_data_(nullptr), connected_(false), sequence_(0) {}
    ~ServiceConnection() { Disconnect(); }

    bool QueryStaticMetadata();

    // Template implementation for input state queries
    template <protocol::MessageType MsgType, typename ResponseType, typename ValueType>
    bool GetInputState(const char* user_path, const char* component_path, int64_t predicted_time, ValueType& out_value);

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
