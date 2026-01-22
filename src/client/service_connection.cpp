#include "service_connection.h"

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#include "../logging.h"

namespace ox {
namespace client {

bool ServiceConnection::Connect() {
    if (connected_) {
        return true;
    }

    LOG_INFO("Connecting to ox-service...");

    // Open shared memory (service must be already running)
    if (!shared_mem_.Create("ox_runtime_shm", sizeof(protocol::SharedData), false)) {
        LOG_ERROR("Failed to open shared memory - is ox-service running?");
        return false;
    }

    shared_data_ = static_cast<protocol::SharedData*>(shared_mem_.GetPointer());

    // Verify protocol version
    uint32_t version = shared_data_->fields.protocol_version.load(std::memory_order_acquire);
    if (version != protocol::PROTOCOL_VERSION) {
        std::ostringstream error_msg;
        error_msg << "Protocol version mismatch - service: " << version
                  << ", client expects: " << protocol::PROTOCOL_VERSION;
        LOG_ERROR(error_msg.str().c_str());
        return false;
    }

    // Connect control channel
    if (!control_.Connect("ox_runtime_control", 5000)) {
        LOG_ERROR("Failed to connect to control channel - is ox-service running?");
        return false;
    }

    // Send connect message
    if (!SendRequest(protocol::MessageType::CONNECT)) {
        LOG_ERROR("Failed to send connect message");
        return false;
    }

    // Query static metadata (cached for the session)
    if (!QueryStaticMetadata()) {
        LOG_ERROR("Failed to query static metadata from service");
        return false;
    }

    connected_ = true;
    shared_data_->fields.client_connected.store(1, std::memory_order_release);

    LOG_INFO("Connected to ox-service successfully");
    return true;
}

void ServiceConnection::Disconnect() {
    if (!connected_) {
        return;
    }

    LOG_INFO("Disconnecting from ox-service...");

    SendRequest(protocol::MessageType::DISCONNECT);

    if (shared_data_) {
        shared_data_->fields.client_connected.store(0, std::memory_order_release);
    }

    control_.Close();
    shared_mem_.Close();
    connected_ = false;
}

bool ServiceConnection::SendRequest(protocol::MessageType type, const void* payload, uint32_t payload_size) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    protocol::MessageHeader request;
    request.type = type;
    request.sequence = sequence_++;
    request.payload_size = payload_size;

    if (!control_.Send(request, payload)) {
        return false;
    }

    // Wait for response
    protocol::MessageHeader response;
    std::vector<uint8_t> response_payload;

    if (!control_.Receive(response, response_payload)) {
        return false;
    }

    return response.type == protocol::MessageType::RESPONSE;
}

uint64_t ServiceConnection::AllocateHandle(protocol::HandleType type) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    protocol::AllocateHandleRequest request;
    request.handle_type = type;

    protocol::MessageHeader header;
    header.type = protocol::MessageType::ALLOCATE_HANDLE;
    header.sequence = sequence_++;
    header.payload_size = sizeof(request);

    if (!control_.Send(header, &request)) {
        return 0;
    }

    // Wait for response
    protocol::MessageHeader response;
    std::vector<uint8_t> response_payload;

    if (!control_.Receive(response, response_payload)) {
        return 0;
    }

    if (response.type == protocol::MessageType::RESPONSE &&
        response_payload.size() >= sizeof(protocol::AllocateHandleResponse)) {
        const protocol::AllocateHandleResponse* resp =
            reinterpret_cast<const protocol::AllocateHandleResponse*>(response_payload.data());
        return resp->handle;
    }

    return 0;
}

bool ServiceConnection::GetNextEvent(protocol::SessionStateEvent& event) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    protocol::MessageHeader request;
    request.type = protocol::MessageType::GET_NEXT_EVENT;
    request.sequence = sequence_++;
    request.payload_size = 0;

    if (!control_.Send(request)) {
        return false;
    }

    // Wait for response
    protocol::MessageHeader response;
    std::vector<uint8_t> response_payload;

    if (!control_.Receive(response, response_payload)) {
        return false;
    }

    if (response.type == protocol::MessageType::RESPONSE &&
        response_payload.size() >= sizeof(protocol::SessionStateEvent)) {
        std::memcpy(&event, response_payload.data(), sizeof(event));
        return true;
    }

    return false;
}

bool ServiceConnection::QueryStaticMetadata() {
    std::lock_guard<std::mutex> lock(send_mutex_);

    // Query runtime properties
    {
        protocol::MessageHeader request;
        request.type = protocol::MessageType::GET_RUNTIME_PROPERTIES;
        request.sequence = sequence_++;
        request.payload_size = 0;

        if (!control_.Send(request)) {
            return false;
        }

        protocol::MessageHeader response;
        std::vector<uint8_t> response_payload;

        if (!control_.Receive(response, response_payload) || response.type != protocol::MessageType::RESPONSE ||
            response_payload.size() < sizeof(protocol::RuntimePropertiesResponse)) {
            return false;
        }

        std::memcpy(&runtime_props_, response_payload.data(), sizeof(runtime_props_));
    }

    // Query system properties
    {
        protocol::MessageHeader request;
        request.type = protocol::MessageType::GET_SYSTEM_PROPERTIES;
        request.sequence = sequence_++;
        request.payload_size = 0;

        if (!control_.Send(request)) {
            return false;
        }

        protocol::MessageHeader response;
        std::vector<uint8_t> response_payload;

        if (!control_.Receive(response, response_payload) || response.type != protocol::MessageType::RESPONSE ||
            response_payload.size() < sizeof(protocol::SystemPropertiesResponse)) {
            return false;
        }

        std::memcpy(&system_props_, response_payload.data(), sizeof(system_props_));
    }

    // Query view configurations
    {
        protocol::MessageHeader request;
        request.type = protocol::MessageType::GET_VIEW_CONFIGURATIONS;
        request.sequence = sequence_++;
        request.payload_size = 0;

        if (!control_.Send(request)) {
            return false;
        }

        protocol::MessageHeader response;
        std::vector<uint8_t> response_payload;

        if (!control_.Receive(response, response_payload) || response.type != protocol::MessageType::RESPONSE ||
            response_payload.size() < sizeof(protocol::ViewConfigurationsResponse)) {
            return false;
        }

        std::memcpy(&view_configs_, response_payload.data(), sizeof(view_configs_));
    }

    // Query interaction profiles
    {
        protocol::MessageHeader request;
        request.type = protocol::MessageType::GET_INTERACTION_PROFILES;
        request.sequence = sequence_++;
        request.payload_size = 0;

        if (!control_.Send(request)) {
            return false;
        }

        protocol::MessageHeader response;
        std::vector<uint8_t> response_payload;

        if (!control_.Receive(response, response_payload) || response.type != protocol::MessageType::RESPONSE ||
            response_payload.size() < sizeof(protocol::InteractionProfilesResponse)) {
            return false;
        }

        std::memcpy(&interaction_profiles_, response_payload.data(), sizeof(interaction_profiles_));
    }

    LOG_INFO("Successfully queried static metadata from service");
    return true;
}

template <protocol::MessageType MsgType, typename ResponseType, typename ValueType>
bool ServiceConnection::GetInputState(const char* user_path, const char* component_path, int64_t predicted_time,
                                      ValueType& out_value) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    // Prepare the request
    protocol::InputStateRequest request;
    request.predicted_time = predicted_time;
    std::strncpy(request.user_path, user_path, sizeof(request.user_path) - 1);
    request.user_path[sizeof(request.user_path) - 1] = '\0';
    std::strncpy(request.component_path, component_path, sizeof(request.component_path) - 1);
    request.component_path[sizeof(request.component_path) - 1] = '\0';

    protocol::MessageHeader header;
    header.type = MsgType;
    header.sequence = sequence_++;
    header.payload_size = sizeof(request);

    // Send the request
    if (!control_.Send(header, &request)) {
        return false;
    }

    // Wait for response
    protocol::MessageHeader response;
    std::vector<uint8_t> response_payload;
    if (!control_.Receive(response, response_payload)) {
        return false;
    }

    // Read the response
    if (response.type == protocol::MessageType::RESPONSE && response_payload.size() >= sizeof(ResponseType)) {
        ResponseType response_data;
        std::memcpy(&response_data, response_payload.data(), sizeof(response_data));
        out_value = response_data.value;
        return true;
    }

    return false;
}

bool ServiceConnection::GetInputStateBoolean(const char* user_path, const char* component_path, int64_t predicted_time,
                                             XrBool32& out_value) {
    return GetInputState<protocol::MessageType::GET_INPUT_STATE_BOOLEAN, protocol::InputStateBooleanResponse>(
        user_path, component_path, predicted_time, out_value);
}

bool ServiceConnection::GetInputStateFloat(const char* user_path, const char* component_path, int64_t predicted_time,
                                           float& out_value) {
    return GetInputState<protocol::MessageType::GET_INPUT_STATE_FLOAT, protocol::InputStateFloatResponse>(
        user_path, component_path, predicted_time, out_value);
}

bool ServiceConnection::GetInputStateVector2f(const char* user_path, const char* component_path, int64_t predicted_time,
                                              XrVector2f& out_value) {
    return GetInputState<protocol::MessageType::GET_INPUT_STATE_VECTOR2F, protocol::InputStateVector2fResponse>(
        user_path, component_path, predicted_time, out_value);
}

}  // namespace client
}  // namespace ox
