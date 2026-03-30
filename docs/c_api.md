# C API

The simulator exports a C API from the same shared library that provides the `ox_driver_register` entry point used by `ox-runtime`.

- Public header: https://github.com/ox-runtime/ox-sim-driver/blob/main/include/ox_sim.h
- Example C++ client: https://github.com/ox-runtime/ox-sim-driver/blob/main/examples/simulator_api_example.cpp
- Example Python `ctypes` client: https://github.com/ox-runtime/ox-sim-driver/blob/main/examples/simulator_control_example.py

The C API is intended for bindings, tests, and local automation. The GUI and the REST API both use this same surface.

## Result Codes

Most functions return `OxSimResult`:

- `OX_SIM_SUCCESS`
- `OX_SIM_ERROR_INVALID_ARGUMENT`
- `OX_SIM_ERROR_NOT_INITIALIZED`
- `OX_SIM_ERROR_BUFFER_TOO_SMALL`
- `OX_SIM_ERROR_PROFILE_NOT_FOUND`
- `OX_SIM_ERROR_DEVICE_NOT_FOUND`
- `OX_SIM_ERROR_COMPONENT_NOT_FOUND`

## Core Types

`OxSimFramePreview` exposes the latest submitted eye textures:

```c
typedef struct {
    const void* pixel_data[2];
    uint32_t data_size[2];
    uint32_t width;
    uint32_t height;
    uint32_t app_fps;
    XrSessionState session_state;
    XrTime frame_time;
} OxSimFramePreview;
```

The pixel buffers are top-down RGBA8 eye images for left eye `0` and right eye `1`, with alpha normalized to opaque so GUI, REST, and clipboard consumers all receive the same preview data.

## Lifecycle

Initialize the process-global simulator state before using the API, and shut it down when finished.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_initialize(void);
OX_DRIVER_EXPORT void ox_sim_shutdown(void);
```

## Profile Management

Read or switch the active simulated device profile.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_current_profile(char* out_name, uint32_t out_name_capacity);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_current_profile(const char* profile_name);
```

Supported profile IDs:

- `oculus_quest_2`
- `oculus_quest_3`
- `htc_vive`
- `valve_index`
- `htc_vive_tracker`

## Device Enumeration And Pose

Enumerate devices and read or write a device pose by user path.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_device_count(uint32_t* out_count);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_device_state(uint32_t device_index, OxDeviceState* out_state);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_device_pose(const char* user_path, XrPosef* out_pose, XrBool32* out_is_active);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_device_pose(const char* user_path, const XrPosef* pose, XrBool32 is_active);
```

Common user paths include:

- `/user/head`
- `/user/hand/left`
- `/user/hand/right`
- `/user/vive_tracker_htcx/role/waist`

## Input State

Read or write boolean, float, and `vec2` input components.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_input_state_boolean(const char* user_path, const char* component_path, uint32_t* out_value);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_input_state_boolean(const char* user_path, const char* component_path, uint32_t value);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_input_state_float(const char* user_path, const char* component_path, float* out_value);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_input_state_float(const char* user_path, const char* component_path, float value);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_input_state_vector2f(const char* user_path, const char* component_path, XrVector2f* out_value);
OX_DRIVER_EXPORT OxSimResult ox_sim_set_input_state_vector2f(const char* user_path, const char* component_path, const XrVector2f* value);
```

Example component paths:

- `/input/trigger/value`
- `/input/trigger/touch`
- `/input/thumbstick`
- `/input/thumbstick/x`
- `/input/a/click`

Available components depend on the active device profile.

## Session And Frame Preview

Inspect current session state, app FPS, and the latest submitted eye textures.

```c
OX_DRIVER_EXPORT OxSimResult ox_sim_get_session_state(XrSessionState* out_state);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_app_fps(uint32_t* out_fps);
OX_DRIVER_EXPORT OxSimResult ox_sim_get_frame_preview(OxSimFramePreview* out_preview);
```

`ox_sim_get_frame_preview` returns pointers into simulator-owned memory. Treat those buffers as read-only and consume them immediately.

## Minimal Example

```cpp
#include <ox_sim.h>

#include <iostream>

int main() {
    if (ox_sim_initialize() != OX_SIM_SUCCESS) {
        return 1;
    }

    ox_sim_set_current_profile("oculus_quest_2");

    XrPosef left_pose = {{0.0f, 0.0f, 0.0f, 1.0f}, {-0.25f, 1.35f, -0.4f}};
    ox_sim_set_device_pose("/user/hand/left", &left_pose, XR_TRUE);
    ox_sim_set_input_state_float("/user/hand/left", "/input/trigger/value", 0.75f);

    float trigger_value = 0.0f;
    if (ox_sim_get_input_state_float("/user/hand/left", "/input/trigger/value", &trigger_value) ==
        OX_SIM_SUCCESS) {
        std::cout << "Left trigger: " << trigger_value << '\n';
    }

    OxSimFramePreview preview = {};
    if (ox_sim_get_frame_preview(&preview) == OX_SIM_SUCCESS && preview.pixel_data[0] != nullptr) {
        std::cout << "Left eye preview: " << preview.width << "x" << preview.height << '\n';
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