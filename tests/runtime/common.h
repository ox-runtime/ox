#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <openxr/openxr.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../../src/client/service_connection.h"
#include "../../src/protocol/messages.h"

// Declare the injection function
extern "C" void oxSetServiceConnection(ox::client::IServiceConnection* service);

namespace ox {
namespace test {

class MockServiceConnection : public client::IServiceConnection {
   public:
    MOCK_METHOD(bool, Connect, (), (override));
    MOCK_METHOD(void, Disconnect, (), (override));
    MOCK_METHOD(bool, IsConnected, (), (const, override));
    MOCK_METHOD(protocol::SharedData*, GetSharedData, (), (override));
    MOCK_METHOD(protocol::ControlChannel&, GetControlChannel, (), (override));
    MOCK_METHOD(bool, SendRequest, (protocol::MessageType, const void*, uint32_t), (override));
    MOCK_METHOD(uint64_t, AllocateHandle, (protocol::HandleType), (override));
    MOCK_METHOD(bool, GetNextEvent, (protocol::SessionStateEvent&), (override));
    MOCK_METHOD(const protocol::RuntimePropertiesResponse&, GetRuntimeProperties, (), (const, override));
    MOCK_METHOD(const protocol::SystemPropertiesResponse&, GetSystemProperties, (), (const, override));
    MOCK_METHOD(const protocol::ViewConfigurationsResponse&, GetViewConfigurations, (), (const, override));
    MOCK_METHOD(const protocol::InteractionProfilesResponse&, GetInteractionProfiles, (), (const, override));
    MOCK_METHOD(bool, GetInputStateBoolean, (const char*, const char*, int64_t, XrBool32&), (override));
    MOCK_METHOD(bool, GetInputStateFloat, (const char*, const char*, int64_t, float&), (override));
    MOCK_METHOD(bool, GetInputStateVector2f, (const char*, const char*, int64_t, XrVector2f&), (override));

    static void SetupDefaultBehaviors(MockServiceConnection* mock) {
        // Initialize shared data
        shared_data_.protocol_version.store(ox::protocol::PROTOCOL_VERSION);
        shared_data_.service_ready.store(1);
        shared_data_.client_connected.store(1);
        shared_data_.session_state.store(static_cast<uint32_t>(ox::protocol::SessionState::READY));
        shared_data_.active_session_handle.store(1000);

        // Default connection behavior
        ON_CALL(*mock, Connect()).WillByDefault(testing::Return(true));
        ON_CALL(*mock, IsConnected()).WillByDefault(testing::Return(true));
        ON_CALL(*mock, GetSharedData()).WillByDefault(testing::Return(&shared_data_));
        ON_CALL(*mock, GetControlChannel()).WillByDefault(testing::ReturnRef(dummy_control_channel_));
        ON_CALL(*mock, SendRequest(ox::protocol::MessageType::CREATE_SESSION, testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::Invoke([](ox::protocol::MessageType, const void*, uint32_t) {
                                              // Simulate service setting the session handle
                                              shared_data_.active_session_handle.store(1000);
                                          }),
                                          testing::Return(true)));
        ON_CALL(*mock, SendRequest(testing::_, testing::_, testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*mock, AllocateHandle(testing::_)).WillByDefault(testing::Return(1000));
        ON_CALL(*mock, GetNextEvent(testing::_)).WillByDefault(testing::Return(false));

        // Default runtime properties
        ON_CALL(*mock, GetRuntimeProperties()).WillByDefault(testing::ReturnRef(runtime_props_));

        // Default system properties
        ON_CALL(*mock, GetSystemProperties()).WillByDefault(testing::ReturnRef(system_props_));

        // Default view configurations
        ON_CALL(*mock, GetViewConfigurations()).WillByDefault(testing::ReturnRef(view_configs_));
        ON_CALL(*mock, GetViewConfigurations()).WillByDefault(testing::ReturnRef(view_configs_));

        // Default interaction profiles
        ON_CALL(*mock, GetInteractionProfiles()).WillByDefault(testing::ReturnRef(interaction_profiles_));

        // Default input state methods
        ON_CALL(*mock, GetInputStateBoolean(testing::_, testing::_, testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgReferee<3>(XR_FALSE), testing::Return(true)));
        ON_CALL(*mock, GetInputStateFloat(testing::_, testing::_, testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgReferee<3>(0.0f), testing::Return(true)));
        ON_CALL(*mock, GetInputStateVector2f(testing::_, testing::_, testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgReferee<3>(XrVector2f{0.0f, 0.0f}), testing::Return(true)));
    }

   private:
    // Default response data
    static const ox::protocol::RuntimePropertiesResponse runtime_props_;
    static const ox::protocol::SystemPropertiesResponse system_props_;
    static const ox::protocol::ViewConfigurationsResponse view_configs_;
    static const ox::protocol::InteractionProfilesResponse interaction_profiles_;
    static ox::protocol::ControlChannel dummy_control_channel_;
    static ox::protocol::SharedData shared_data_;
};

// Base test fixture for runtime tests
class RuntimeTestBase : public ::testing::Test {
   protected:
    void SetUp() override {
        mock_service = std::make_unique<ox::test::MockServiceConnection>();

        ox::test::MockServiceConnection::SetupDefaultBehaviors(mock_service.get());

        oxSetServiceConnection(mock_service.get());
    }

    void TearDown() override {
        oxSetServiceConnection(nullptr);
        mock_service.reset();
    }

    // Helper to create a basic instance
    XrInstance CreateBasicInstance(const std::string& app_name = "TestApp") {
        XrInstanceCreateInfo create_info{XR_TYPE_INSTANCE_CREATE_INFO};
        std::strncpy(create_info.applicationInfo.applicationName, app_name.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
        create_info.applicationInfo.applicationVersion = 1;
        std::strncpy(create_info.applicationInfo.engineName, "TestEngine", XR_MAX_ENGINE_NAME_SIZE);
        create_info.applicationInfo.engineVersion = 1;
        create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

        XrInstance instance = XR_NULL_HANDLE;
        XrResult result = xrCreateInstance(&create_info, &instance);

        if (result == XR_SUCCESS && instance != XR_NULL_HANDLE) {
            created_instances_.push_back(instance);
        }

        return instance;
    }

    std::vector<XrInstance> created_instances_;
    std::unique_ptr<ox::test::MockServiceConnection> mock_service;
};

}  // namespace test
}  // namespace ox
