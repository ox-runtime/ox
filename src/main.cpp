#include <atomic>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>

#include <csignal>
#ifdef __linux__
#include <linux/limits.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#endif
#endif

#include <ox_driver.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace {

std::atomic<bool> g_running{true};

#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD signal) {
    switch (signal) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            g_running.store(false, std::memory_order_release);
            return TRUE;
        default:
            return FALSE;
    }
}
#else
void SignalHandler(int) { g_running.store(false, std::memory_order_release); }
#endif

std::string DriverLibraryFilename() {
#ifdef _WIN32
    return "ox_driver.dll";
#elif defined(__APPLE__)
    return "libox_driver.dylib";
#else
    return "libox_driver.so";
#endif
}

std::string BackendLibraryFilename() {
#ifdef _WIN32
    return "ox_ipc_backend.dll";
#elif defined(__APPLE__)
    return "libox_ipc_backend.dylib";
#else
    return "libox_ipc_backend.so";
#endif
}

fs::path ExecutableDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    return len > 0 ? fs::path(std::string(path, len)).parent_path() : fs::current_path();
#elif defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return fs::weakly_canonical(fs::path(path)).parent_path();
    }
    return fs::current_path();
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return fs::path(path).parent_path();
    }
    return fs::current_path();
#endif
}

std::vector<fs::path> DiscoverDriverDirectories(const fs::path& drivers_dir) {
    std::vector<fs::path> drivers;
    if (!fs::exists(drivers_dir)) {
        return drivers;
    }

    const std::string library_name = DriverLibraryFilename();
    for (const auto& entry : fs::directory_iterator(drivers_dir)) {
        if (entry.is_directory() && fs::exists(entry.path() / library_name)) {
            drivers.push_back(entry.path());
        }
    }
    return drivers;
}

// Holds a loaded driver library and its registered callbacks.
struct DriverHandle {
    void* library = nullptr;
    OxDriverCallbacks callbacks = {};

    ~DriverHandle() { Unload(); }

    void Unload() {
        if (library) {
            if (callbacks.shutdown) {
                callbacks.shutdown();
            }
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(library));
#else
            dlclose(library);
#endif
            library = nullptr;
        }
        callbacks = {};
    }
};

bool LoadDriver(const fs::path& driver_dir, DriverHandle& out) {
    const fs::path lib_path = driver_dir / DriverLibraryFilename();

#ifdef _WIN32
    out.library = LoadLibraryA(lib_path.string().c_str());
    if (!out.library) {
        spdlog::error("Failed to load driver library: {}", lib_path.string());
        return false;
    }
    auto register_func =
        reinterpret_cast<OxDriverRegisterFunc>(GetProcAddress(static_cast<HMODULE>(out.library), "ox_driver_register"));
#else
    out.library = dlopen(lib_path.string().c_str(), RTLD_NOW);
    if (!out.library) {
        spdlog::error("Failed to load driver library: {}", dlerror());
        return false;
    }
    auto register_func = reinterpret_cast<OxDriverRegisterFunc>(dlsym(out.library, "ox_driver_register"));
#endif

    if (!register_func) {
        spdlog::error("Failed to find ox_driver_register in {}", lib_path.string());
        out.Unload();
        return false;
    }

    if (!register_func(&out.callbacks)) {
        spdlog::error("Driver registration failed");
        out.Unload();
        return false;
    }

    if (!out.callbacks.initialize || !out.callbacks.is_device_connected || !out.callbacks.update_view_pose) {
        spdlog::error("Driver missing required callbacks");
        out.Unload();
        return false;
    }

    if (!out.callbacks.initialize()) {
        spdlog::error("Driver initialize() failed");
        out.Unload();
        return false;
    }

    if (!out.callbacks.is_device_connected()) {
        spdlog::error("Driver reported no connected device");
        out.Unload();
        return false;
    }

    spdlog::info("Driver loaded: {}", lib_path.string());
    return true;
}

class BackendModule {
   public:
    using SetDriverFunc = void (*)(const OxDriverCallbacks*);
    using InitializeFunc = int (*)();
    using ShutdownFunc = void (*)();

    BackendModule() = default;
    ~BackendModule() { Unload(); }

    bool Load(const fs::path& module_path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(module_path.string().c_str());
        if (!handle_) {
            spdlog::error("Failed to load ox_ipc_backend: {}", module_path.string());
            return false;
        }
        set_driver_ = reinterpret_cast<SetDriverFunc>(GetProcAddress(static_cast<HMODULE>(handle_), "set_driver"));
        initialize_ = reinterpret_cast<InitializeFunc>(GetProcAddress(static_cast<HMODULE>(handle_), "initialize"));
        shutdown_ = reinterpret_cast<ShutdownFunc>(GetProcAddress(static_cast<HMODULE>(handle_), "shutdown"));
#else
        handle_ = dlopen(module_path.string().c_str(), RTLD_NOW);
        if (!handle_) {
            spdlog::error("Failed to load ox_ipc_backend: {}", dlerror());
            return false;
        }
        set_driver_ = reinterpret_cast<SetDriverFunc>(dlsym(handle_, "set_driver"));
        initialize_ = reinterpret_cast<InitializeFunc>(dlsym(handle_, "initialize"));
        shutdown_ = reinterpret_cast<ShutdownFunc>(dlsym(handle_, "shutdown"));
#endif

        if (!set_driver_ || !initialize_ || !shutdown_) {
            spdlog::error("ox_ipc_backend is missing required exports");
            Unload();
            return false;
        }

        return true;
    }

    void SetDriver(const OxDriverCallbacks* cb) const { set_driver_(cb); }
    bool Initialize() const { return initialize_ && initialize_() == 1; }
    void Shutdown() const {
        if (shutdown_) {
            shutdown_();
        }
    }

    void Unload() {
        if (handle_) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(handle_));
#else
            dlclose(handle_);
#endif
            handle_ = nullptr;
        }
        set_driver_ = nullptr;
        initialize_ = nullptr;
        shutdown_ = nullptr;
    }

   private:
    void* handle_ = nullptr;
    SetDriverFunc set_driver_ = nullptr;
    InitializeFunc initialize_ = nullptr;
    ShutdownFunc shutdown_ = nullptr;
};

bool RunHost() {
    const fs::path exe_dir = ExecutableDirectory();
    const fs::path drivers_dir = exe_dir / "drivers";

    auto driver_dirs = DiscoverDriverDirectories(drivers_dir);
    if (driver_dirs.empty()) {
        spdlog::error("No driver folders found in {}", drivers_dir.string());
        return false;
    }

    if (driver_dirs.size() > 1) {
        std::ostringstream msg;
        msg << "Multiple drivers found in " << drivers_dir << ":";
        for (const auto& d : driver_dirs) {
            msg << " " << d.filename().string();
        }
        spdlog::error("{}", msg.str());
        return false;
    }

    DriverHandle driver;
    if (!LoadDriver(driver_dirs[0], driver)) {
        return false;
    }

    BackendModule backend;
    if (!backend.Load(exe_dir / BackendLibraryFilename())) {
        return false;
    }

    backend.SetDriver(&driver.callbacks);
    if (!backend.Initialize()) {
        spdlog::error("Failed to initialize ox_ipc_backend");
        backend.Unload();
        return false;
    }

    spdlog::info("ox host started");
    while (g_running.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    backend.Shutdown();
    backend.Unload();
    spdlog::info("ox host stopped");
    return true;
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#else
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
#endif

    return RunHost() ? 0 : 1;
}
