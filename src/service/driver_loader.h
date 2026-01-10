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
        lib_filename = "driver.dll";
#else
        lib_filename = "libdriver.so";
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
        if (!callbacks_.initialize || !callbacks_.is_device_connected || !callbacks_.update_pose ||
            !callbacks_.update_view_pose) {
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

    void UpdatePose(int64_t predicted_time, OxPose* out_pose) const {
        if (loaded_ && callbacks_.update_pose) {
            callbacks_.update_pose(predicted_time, out_pose);
        }
    }

    void UpdateViewPose(int64_t predicted_time, uint32_t eye_index, OxPose* out_pose) const {
        if (loaded_ && callbacks_.update_view_pose) {
            callbacks_.update_view_pose(predicted_time, eye_index, out_pose);
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
