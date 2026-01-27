#pragma once

#include <openxr/openxr.h>

#include "../protocol/control_channel.h"
#include "../protocol/messages.h"

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

}  // namespace client
}  // namespace ox
