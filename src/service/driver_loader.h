#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <ox_driver.h>

#include "../logging.h"

namespace ox {
namespace driver {

class DriverLoader {
   public:
    DriverLoader() : driver_handle_(nullptr), callbacks_{}, loaded_(false) {}

    ~DriverLoader() { Unload(); }

    // Load driver from a specific directory
    bool LoadDriver(const std::string& driver_path) {
        if (loaded_) {
            LOG_ERROR("Driver already loaded");
            return false;
        }

        // Look for standardized driver library name
        std::string lib_filename;
#ifdef _WIN32
        lib_filename = "ox_driver.dll";
#elif defined(__APPLE__)
        lib_filename = "libox_driver.dylib";
#else
        lib_filename = "libox_driver.so";
#endif

        std::string lib_path = driver_path;
#ifdef _WIN32
        lib_path += "\\" + lib_filename;
#else
        lib_path += "/" + lib_filename;
#endif

#ifdef _WIN32
        driver_handle_ = LoadLibraryA(lib_path.c_str());
        if (!driver_handle_) {
            LOG_ERROR(("Failed to load driver library: " + lib_path).c_str());
            return false;
        }

        OxDriverRegisterFunc register_func =
            (OxDriverRegisterFunc)GetProcAddress((HMODULE)driver_handle_, "ox_driver_register");
#else
        driver_handle_ = dlopen(lib_path.c_str(), RTLD_NOW);
        if (!driver_handle_) {
            LOG_ERROR((std::string("Failed to load driver library: ") + dlerror()).c_str());
            return false;
        }

        OxDriverRegisterFunc register_func = (OxDriverRegisterFunc)dlsym(driver_handle_, "ox_driver_register");
#endif

        if (!register_func) {
            LOG_ERROR("Failed to find ox_driver_register function");
            Unload();
            return false;
        }

        // Call the driver's register function
        if (!register_func(&callbacks_)) {
            LOG_ERROR("Driver registration failed");
            Unload();
            return false;
        }

        // Verify all required callbacks are present
        if (!callbacks_.initialize || !callbacks_.is_device_connected || !callbacks_.update_view_pose) {
            LOG_ERROR("Driver missing required callbacks");
            Unload();
            return false;
        }

        // Initialize the driver
        if (!callbacks_.initialize()) {
            LOG_ERROR("Driver initialization failed");
            Unload();
            return false;
        }

        loaded_ = true;
        LOG_INFO(("Driver loaded successfully: " + lib_path).c_str());
        return true;
    }

    void Unload() {
        if (loaded_ && callbacks_.shutdown) {
            callbacks_.shutdown();
        }

        if (driver_handle_) {
#ifdef _WIN32
            FreeLibrary((HMODULE)driver_handle_);
#else
            dlclose(driver_handle_);
#endif
            driver_handle_ = nullptr;
        }

        loaded_ = false;
    }

    bool IsDeviceConnected() const {
        if (!loaded_ || !callbacks_.is_device_connected) {
            return false;
        }
        return callbacks_.is_device_connected();
    }

    void GetDeviceInfo(OxDeviceInfo* info) const {
        if (loaded_ && callbacks_.get_device_info) {
            callbacks_.get_device_info(info);
        }
    }

    void GetDisplayProperties(OxDisplayProperties* props) const {
        if (loaded_ && callbacks_.get_display_properties) {
            callbacks_.get_display_properties(props);
        }
    }

    void GetTrackingCapabilities(OxTrackingCapabilities* caps) const {
        if (loaded_ && callbacks_.get_tracking_capabilities) {
            callbacks_.get_tracking_capabilities(caps);
        }
    }

    void UpdateViewPose(int64_t predicted_time, uint32_t eye_index, OxPose* out_pose) const {
        if (loaded_ && callbacks_.update_view_pose) {
            callbacks_.update_view_pose(predicted_time, eye_index, out_pose);
        }
    }

    bool HasUpdateDevices() const { return loaded_ && callbacks_.update_devices; }

    void UpdateDevices(int64_t predicted_time, OxDeviceState* out_states, uint32_t* out_count) const {
        if (loaded_ && callbacks_.update_devices) {
            callbacks_.update_devices(predicted_time, out_states, out_count);
        } else {
            // Driver doesn't support devices
            *out_count = 0;
        }
    }

    std::vector<std::string> GetInteractionProfiles() const {
        std::vector<std::string> profiles;
        if (loaded_ && callbacks_.get_interaction_profiles) {
            const char* profile_ptrs[16];
            uint32_t count = callbacks_.get_interaction_profiles(profile_ptrs, 16);
            for (uint32_t i = 0; i < count && i < 16; i++) {
                if (profile_ptrs[i]) {
                    profiles.push_back(profile_ptrs[i]);
                }
            }
        }
        return profiles;
    }

    uint32_t GetInputStateBoolean(int64_t predicted_time, const char* user_path, const char* component_path,
                                  uint32_t* out_value) const {
        if (loaded_ && callbacks_.get_input_state_boolean) {
            OxComponentResult result =
                callbacks_.get_input_state_boolean(predicted_time, user_path, component_path, out_value);
            return (result == OX_COMPONENT_AVAILABLE) ? 1 : 0;
        }
        return 0;
    }

    uint32_t GetInputStateFloat(int64_t predicted_time, const char* user_path, const char* component_path,
                                float* out_value) const {
        if (loaded_ && callbacks_.get_input_state_float) {
            OxComponentResult result =
                callbacks_.get_input_state_float(predicted_time, user_path, component_path, out_value);
            return (result == OX_COMPONENT_AVAILABLE) ? 1 : 0;
        }
        return 0;
    }

    uint32_t GetInputStateVector2f(int64_t predicted_time, const char* user_path, const char* component_path,
                                   float* out_x, float* out_y) const {
        if (loaded_ && callbacks_.get_input_state_vector2f) {
            OxVector2f vec;
            OxComponentResult result =
                callbacks_.get_input_state_vector2f(predicted_time, user_path, component_path, &vec);
            if (result == OX_COMPONENT_AVAILABLE) {
                *out_x = vec.x;
                *out_y = vec.y;
                return 1;
            }
        }
        return 0;
    }

    bool HasSubmitFramePixels() const { return loaded_ && callbacks_.submit_frame_pixels; }

    void SubmitFramePixels(uint32_t eye_index, uint32_t width, uint32_t height, uint32_t format, const void* pixel_data,
                           uint32_t data_size) const {
        if (loaded_ && callbacks_.submit_frame_pixels) {
            callbacks_.submit_frame_pixels(eye_index, width, height, format, pixel_data, data_size);
        }
    }

    bool IsLoaded() const { return loaded_; }

   private:
    void* driver_handle_;
    OxDriverCallbacks callbacks_;
    bool loaded_;
};

}  // namespace driver
}  // namespace ox
