#include "service_connection.h"

#include <chrono>
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
    uint32_t version = shared_data_->protocol_version.load(std::memory_order_acquire);
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

    connected_ = true;
    shared_data_->client_connected.store(1, std::memory_order_release);

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
        shared_data_->client_connected.store(0, std::memory_order_release);
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

}  // namespace client
}  // namespace ox
