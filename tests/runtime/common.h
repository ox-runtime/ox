#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <openxr/openxr.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../../src/client/iservice_connection.h"
#include "../../src/protocol/messages.h"

// Declare the injection function
extern "C" void oxSetServiceConnection(ox::client::IServiceConnection* service);

namespace ox {
namespace test {

/**
 * GMock mock for IServiceConnection
 *
 * This mock replaces the custom implementation to allow testing the OpenXR runtime logic
 * without requiring the actual ox-service to be running.
 */
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

    // Static method to set up default mock behaviors
    static void SetupDefaultBehaviors(MockServiceConnection* mock) {
        // Set up default return values for methods that might be called
        ON_CALL(*mock, Connect()).WillByDefault(testing::Return(true));
        ON_CALL(*mock, IsConnected()).WillByDefault(testing::Return(true));
        ON_CALL(*mock, GetSharedData()).WillByDefault(testing::Return(nullptr));
        ON_CALL(*mock, GetControlChannel()).WillByDefault(testing::ReturnRef(dummy_control_channel_));
        ON_CALL(*mock, SendRequest(testing::_, testing::_, testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*mock, AllocateHandle(testing::_)).WillByDefault(testing::Return(1000));
        ON_CALL(*mock, GetNextEvent(testing::_)).WillByDefault(testing::Return(false));

        // Set up default runtime properties
        ON_CALL(*mock, GetRuntimeProperties()).WillByDefault(testing::ReturnRef(runtime_props_));

        // Set up default system properties
        ON_CALL(*mock, GetSystemProperties()).WillByDefault(testing::ReturnRef(system_props_));

        // Set up default view configurations
        ON_CALL(*mock, GetViewConfigurations()).WillByDefault(testing::ReturnRef(view_configs_));

        // Set up default interaction profiles
        ON_CALL(*mock, GetInteractionProfiles()).WillByDefault(testing::ReturnRef(interaction_profiles_));

        // Set up default input state methods
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
};

// Static member definitions
const ox::protocol::RuntimePropertiesResponse MockServiceConnection::runtime_props_ = []() {
    ox::protocol::RuntimePropertiesResponse props = {};
    strcpy(props.runtime_name, "ox Mock Runtime");
    props.runtime_version_major = 1;
    props.runtime_version_minor = 0;
    props.runtime_version_patch = 0;
    props.padding = 0;
    return props;
}();

const ox::protocol::SystemPropertiesResponse MockServiceConnection::system_props_ = []() {
    ox::protocol::SystemPropertiesResponse props = {};
    strcpy(props.system_name, "Mock VR System");
    props.max_swapchain_width = 2048;
    props.max_swapchain_height = 2048;
    props.max_layer_count = 16;
    props.orientation_tracking = 1;
    props.position_tracking = 1;
    props.padding[0] = 0;
    props.padding[1] = 0;
    return props;
}();

const ox::protocol::ViewConfigurationsResponse MockServiceConnection::view_configs_ = []() {
    ox::protocol::ViewConfigurationsResponse configs = {};
    configs.views[0].recommended_width = 1832;
    configs.views[0].recommended_height = 1920;
    configs.views[0].recommended_sample_count = 1;
    configs.views[0].max_sample_count = 4;
    configs.views[1].recommended_width = 1832;
    configs.views[1].recommended_height = 1920;
    configs.views[1].recommended_sample_count = 1;
    configs.views[1].max_sample_count = 4;
    return configs;
}();

const ox::protocol::InteractionProfilesResponse MockServiceConnection::interaction_profiles_ = []() {
    ox::protocol::InteractionProfilesResponse profiles = {};
    profiles.profile_count = 1;
    strcpy(profiles.profiles[0], "/interaction_profiles/khr/simple_controller");
    return profiles;
}();

ox::protocol::ControlChannel MockServiceConnection::dummy_control_channel_ = {};

}  // namespace test
}  // namespace ox

class RuntimeTestBase : public ::testing::Test {
   protected:
    void SetUp() override {
        // Initialize and inject mock service connection
        mock_service = std::make_unique<ox::test::MockServiceConnection>();

        // Set up default behaviors for the mock
        ox::test::MockServiceConnection::SetupDefaultBehaviors(mock_service.get());

        oxSetServiceConnection(mock_service.get());
    }

    void TearDown() override {
        oxSetServiceConnection(nullptr);  // Reset to default
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