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
constexpr std::string_view kServerLibraryStem = "ox_ipc_server";

bool IsMacSimulatorDriver(const fs::path& driver_dir) {
#if defined(__APPLE__)
    return driver_dir.filename() == "ox_simulator";
#else
    (void)driver_dir;
    return false;
#endif
}

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
    const fs::path driver_dir = driver_dirs[0];
    const bool start_server_before_driver_init = IsMacSimulatorDriver(driver_dir);
    dylib::library driver_lib((driver_dir / kDriverLibraryStem).string(), dylib::decorations::os_default());
    auto ox_driver_register = driver_lib.get_function<int(OxDriverCallbacks*)>("ox_driver_register");
    OxDriverCallbacks driver{};
    if (!ox_driver_register || !ox_driver_register(&driver)) {
        spdlog::error("Driver registration failed");
        return 1;
    }
    if (!driver.initialize || !driver.is_device_connected || !driver.update_view) {
        spdlog::error("Driver missing required callbacks");
        return 1;
    }
    // Load server
    dylib::library server_lib((curr_dir / kServerLibraryStem).string(), dylib::decorations::os_default());
    auto server_set_driver = server_lib.get_function<void(const OxDriverCallbacks*)>("ox_ipc_server_set_driver");
    auto server_initialize = server_lib.get_function<int()>("ox_ipc_server_initialize");
    auto server_shutdown = server_lib.get_function<void()>("ox_ipc_server_shutdown");

    bool server_started = false;

    if (start_server_before_driver_init) {
        spdlog::info("Starting IPC server before simulator GUI initialization on macOS");
        server_set_driver(&driver);
        if (server_initialize() != 1) {
            spdlog::error("ox_ipc_server initialize() failed");
            return 1;
        }
        server_started = true;
        spdlog::info("ox host started");
    }

    if (!driver.initialize()) {
        spdlog::error("Driver initialize() failed");
        if (server_started) {
            server_shutdown();
        }
        if (driver.shutdown) {
            driver.shutdown();
        }
        return 1;
    }

    if (!server_started) {
        server_set_driver(&driver);
        if (server_initialize() != 1) {
            spdlog::error("ox_ipc_server initialize() failed");
            if (driver.shutdown) {
                driver.shutdown();
            }
            return 1;
        }
        server_started = true;
        spdlog::info("ox host started");
    }

    while (!driver.is_driver_running || driver.is_driver_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    spdlog::info("Driver signaled shutdown. Cleaning up...");

    if (server_started) {
        server_shutdown();
    }
    if (driver.shutdown) {
        driver.shutdown();
    }
    spdlog::info("ox host stopped");
    return 0;
}
