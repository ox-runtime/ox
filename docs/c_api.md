# C API

The simulator exports a C API that applications can use to interact with the simulator.

- Public header: https://github.com/ox-runtime/ox-sim-driver/blob/main/include/ox_sim.h
- Example C++ client: https://github.com/ox-runtime/ox-sim-driver/blob/main/examples/simulator_api_example.cpp
- Example Python `ctypes` client: https://github.com/ox-runtime/ox-sim-driver/blob/main/examples/simulator_control_example.py

The C API is intended for bindings, tests, and local automation. The GUI and the REST API both use this same surface, so all state stays synchronized.

## Result Codes

Most functions return `OxSimResult`:

- `OX_SIM_SUCCESS`
- `OX_SIM_ERROR_INVALID_ARGUMENT`
- `OX_SIM_ERROR_NOT_INITIALIZED`
- `OX_SIM_ERROR_BUFFER_TOO_SMALL`
- `OX_SIM_ERROR_PROFILE_NOT_FOUND`
- `OX_SIM_ERROR_DEVICE_NOT_FOUND`
- `OX_SIM_ERROR_COMPONENT_NOT_FOUND`

## Resource Model

The API mirrors the REST resources exposed by the simulator:

- `Status` -> current session state and app FPS
- `Views` -> latest submitted eye textures
- `Profile` -> active profile ID and metadata
- `Devices` -> static device metadata plus live device state
- `Components` -> static input metadata per device
- `Inputs` -> typed input state accessors

## Core Types

```c
typedef enum {
    OX_SIM_COMPONENT_TYPE_BOOLEAN = 0,
    OX_SIM_COMPONENT_TYPE_FLOAT = 1,
    OX_SIM_COMPONENT_TYPE_VEC2 = 2,
} OxSimComponentType;

typedef struct {
    char name[128];
    char manufacturer[128];
    char interaction_profile[256];
} OxSimProfileInfo;

typedef struct {
    char user_path[256];
    char role[64];
    XrBool32 always_active;
} OxSimDeviceInfo;

typedef struct {
    char path[256];
    OxSimComponentType type;
    char description[128];
} OxSimComponentInfo;

typedef struct {
    XrSessionState session_state;
    XrBool32 session_active;
    uint32_t fps;
} OxSimStatus;

typedef struct {
    uint32_t data_size;
    uint32_t width;
    uint32_t height;
    XrTime frame_time;
} OxSimViewInfo;
```

## Lifecycle

Initialize the process-global simulator state before using the API, and shut it down when finished.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_initialize(void);
OX_DRIVER_EXPORT void ox_sim_shutdown(void);
```

## Status And Views

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_status(OxSimStatus* out_status);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_view_count(uint32_t* out_count);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_view_info(uint32_t eye_index, OxSimViewInfo* out_view);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_view(uint32_t eye_index, void* out_pixels, uint32_t out_pixels_capacity);
```

- `ox_sim_get_view_count` is usually `2`, but returns `0` for tracker-only profiles.
- `ox_sim_get_view_info` returns metadata, including the required RGBA8 buffer size in `data_size`.
- `ox_sim_get_view` copies the latest view pixels into caller-owned memory.
- `ox_sim_get_view` returns `OX_SIM_ERROR_BUFFER_TOO_SMALL` if `out_pixels_capacity` is smaller than `data_size`.
- Both view functions return `OX_SIM_ERROR_NOT_INITIALIZED` when no frame has been submitted yet for that eye.

## Profile

Read or switch the active simulated device profile and fetch its display metadata.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_profile(char* out_id, uint32_t out_id_capacity);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_profile(const char* profile_id);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_profile_info(OxSimProfileInfo* out_info);
```

Supported profile IDs:

- `oculus_quest_2`
- `oculus_quest_3`
- `htc_vive`
- `valve_index`
- `htc_vive_tracker`

## Devices

Enumerate static device metadata and read or write live device state.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_device_count(uint32_t* out_count);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_device_info(uint32_t index, OxSimDeviceInfo* out_info);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_device(const char* user_path, OxDeviceState* out_state);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_device(const char* user_path, const OxDeviceState* state);
```

Common user paths include:

- `/user/head`
- `/user/hand/left`
- `/user/hand/right`
- `/user/vive_tracker_htcx/role/waist`

## Components

Enumerate the static input components available on a given device.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_component_count(const char* user_path, uint32_t* out_count);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_component_info(const char* user_path, uint32_t index,
                                                       OxSimComponentInfo* out_info);
```

## Inputs

Read or write boolean, float, and `vec2` input components using the typed accessors that match the component metadata.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_input_boolean(const char* user_path, const char* component_path,
                                                      XrBool32* out_value);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_input_boolean(const char* user_path, const char* component_path,
                                                      XrBool32 value);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_input_float(const char* user_path, const char* component_path,
                                                    float* out_value);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_input_float(const char* user_path, const char* component_path, float value);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_input_vector2f(const char* user_path, const char* component_path,
                                                       XrVector2f* out_value);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_input_vector2f(const char* user_path, const char* component_path,
                                                       const XrVector2f* value);
```

Example component paths:

- `/input/trigger/value`
- `/input/trigger/touch`
- `/input/thumbstick`
- `/input/thumbstick/x`
- `/input/a/click`

Available components depend on the active device profile.

## Minimal Example

```cpp
#include <ox_sim.h>

#include <iostream>

int main() {
    if (ox_sim_initialize() != OX_SIM_SUCCESS) {
        return 1;
    }

    ox_sim_set_profile("oculus_quest_2");

    OxDeviceState left_state = {};
    left_state.pose = {{0.0f, 0.0f, 0.0f, 1.0f}, {-0.25f, 1.35f, -0.4f}};
    left_state.is_active = XR_TRUE;
    ox_sim_set_device("/user/hand/left", &left_state);
    ox_sim_set_input_float("/user/hand/left", "/input/trigger/value", 0.75f);

    float trigger_value = 0.0f;
    if (ox_sim_get_input_float("/user/hand/left", "/input/trigger/value", &trigger_value) == OX_SIM_SUCCESS) {
        std::cout << "Left trigger: " << trigger_value << '\n';
    }

    OxSimStatus status = {};
    if (ox_sim_get_status(&status) == OX_SIM_SUCCESS) {
        std::cout << "Session state: " << static_cast<uint32_t>(status.session_state) << '\n';
    }

    ox_sim_shutdown();
    return 0;
}
```

## Python `ctypes`

For Python automation, see the `ctypes` example in the ox-sim-driver repository: https://github.com/ox-runtime/ox-sim-driver/blob/main/examples/simulator_control_example.py

## Notes

- The API is process-global.
- The GUI and REST server are started automatically only when the simulator is loaded as a driver.
- When the library is used directly through the C API, you control initialization and shutdown yourself.
