#include <ox_driver.h>
#include <spdlog/spdlog.h>
#include <whereami.h>

#include <chrono>
#include <dylib.hpp>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

constexpr std::string_view kDriverLibraryStem = "ox_driver";
constexpr std::string_view kBackendLibraryStem = "ox_ipc_backend";

fs::path GetCurrentDir() {
    const int len = wai_getModulePath(nullptr, 0, nullptr);
    assert(len > 0 && "Failed to get executable path");

    std::string buf(static_cast<size_t>(len) + 1, '\0');
    wai_getModulePath(buf.data(), len, nullptr);
    buf[static_cast<size_t>(len)] = '\0';

    return fs::path(buf.c_str()).parent_path();
}

int main() {
    const fs::path curr_dir = GetCurrentDir();

    // Find the single driver directory
    const fs::path drivers_dir = curr_dir / "drivers";
    std::vector<fs::path> driver_dirs;
    for (const auto& entry : fs::directory_iterator(drivers_dir)) {
        if (entry.is_directory()) {
            driver_dirs.push_back(entry.path());
        }
    }
    if (driver_dirs.size() != 1) {
        spdlog::error("Expected exactly one driver in {}, found {}", drivers_dir.string(), driver_dirs.size());
        return 1;
    }

    // Load driver
    dylib::library driver_lib((driver_dirs[0] / kDriverLibraryStem).string(), dylib::decorations::os_default());
    auto ox_driver_register = driver_lib.get_function<int(OxDriverCallbacks*)>("ox_driver_register");
    OxDriverCallbacks driver{};
    if (!ox_driver_register || !ox_driver_register(&driver)) {
        spdlog::error("Driver registration failed");
        return 1;
    }
    if (!driver.initialize || !driver.is_device_connected || !driver.update_view_pose) {
        spdlog::error("Driver missing required callbacks");
        return 1;
    }
    if (!driver.initialize()) {
        spdlog::error("Driver initialize() failed");
        return 1;
    }

    // Load backend
    dylib::library backend_lib((curr_dir / kBackendLibraryStem).string(), dylib::decorations::os_default());
    auto backend_set_driver = backend_lib.get_function<void(const OxDriverCallbacks*)>("ox_ipc_backend_set_driver");
    auto backend_initialize = backend_lib.get_function<int()>("ox_ipc_backend_initialize");
    auto backend_shutdown = backend_lib.get_function<void()>("ox_ipc_backend_shutdown");

    backend_set_driver(&driver);
    if (backend_initialize() != 1) {
        spdlog::error("ox_ipc_backend initialize() failed");
        if (driver.shutdown) {
            driver.shutdown();
        }
        return 1;
    }

    spdlog::info("ox host started");
    while (!driver.is_driver_running || driver.is_driver_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    spdlog::info("Driver signaled shutdown. Cleaning up...");

    backend_shutdown();
    if (driver.shutdown) {
        driver.shutdown();
    }
    spdlog::info("ox host stopped");
    return 0;
}
