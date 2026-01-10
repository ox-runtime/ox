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

   private:
    ServiceConnection() : shared_data_(nullptr), connected_(false), sequence_(0) {}
    ~ServiceConnection() { Disconnect(); }

    protocol::SharedMemory shared_mem_;
    protocol::ControlChannel control_;
    protocol::SharedData* shared_data_;
    std::atomic<bool> connected_;
    std::atomic<uint32_t> sequence_;
    std::mutex send_mutex_;
};

}  // namespace client
}  // namespace ox
