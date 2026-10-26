# Simulator

`ox-sim-driver` is the virtual XR device. It is a driver, not a special case inside the runtime. To the runtime, it looks like an `ox_driver.h` library that reports a connected device, poses, input values, interaction profiles, and submitted frames.

Because it follows the same driver contract as real hardware, the runtime does not need simulator-specific code. Adding a new simulated headset is a data change, not a runtime change.

This page covers both how the simulator is built and how to control it.

## Internals

### Where the state lives

[`src/ox_sim.cpp`](https://github.com/ox-runtime/ox-sim-driver/blob/main/src/ox_sim.cpp) owns the state: the active profile, device poses and active flags, per-device input values, the latest frame for each eye, session state, and a short frame-time history for the FPS estimate. A mutex guards the state. The header says all functions are safe to call from any thread.

[`src/driver.cpp`](https://github.com/ox-runtime/ox-sim-driver/blob/main/src/driver.cpp) connects the driver callbacks to that state:

| Driver callback | What the simulator does |
|---|---|
| `initialize` | Calls `ox_sim_initialize`, then starts the HTTP server and GUI if the config enables them. |
| `get_system_properties` | Fills in the name, vendor, swapchain limits, and tracking flags from the active profile. |
| `update_view` | Reads `/user/head`, offsets each eye by half the IPD, and uses the profile field of view. |
| `update_devices` | Copies all active device states. |
| `get_input_state_*` | Reads the matching component from simulator state. |
| `get_interaction_profiles` | Returns the profile's interaction profile path. |
| `submit_frame_pixels` | Stores the frame for the GUI, C API, and REST API. |
| `on_session_state_changed` | Updates session status. |

The GUI runs its own window loop. `is_driver_running` returns false when the GUI window closes, which tells the `ox` host to shut down.

### Device profiles

[`src/device_profiles.hpp`](https://github.com/ox-runtime/ox-sim-driver/blob/main/src/device_profiles.hpp) lists the simulated hardware. A `DeviceProfile` describes:

- name, manufacturer, serial prefix, and vendor and product IDs;
- per-eye width and height, recommended size, refresh rate, and field of view;
- tracking support;
- the interaction profile path;
- the devices and their default poses.

A `DeviceDef` describes one device: its user path, role, whether it is always active, its default pose, and its inputs. A `ComponentDef` describes one input path, its type (float, boolean, or vec2), a description, an optional hand restriction, and an optional link from a vec2 axis to its parent vector.

The supported profiles are:

| ID | Device |
|---|---|
| `oculus_quest_2` | Meta Quest 2 with Touch controllers. |
| `oculus_quest_3` | Meta Quest 3 with Touch controllers. |
| `htc_vive` | HTC Vive with wand controllers. |
| `valve_index` | Valve Index with Index controllers. |
| `htc_vive_tracker` | Vive Trackers for the waist, feet, and shoulders. This profile has no views. |

Switching a profile resets poses and inputs to that profile's defaults. The tracker profile has `view_count` zero, which is why `ox_sim_get_view_count` can return zero.

To add another headset to the simulator, add a profile here. Do not write a new driver just to simulate existing hardware.

### The C API

[`ox_sim.h`](https://github.com/ox-runtime/ox-sim-driver/blob/main/include/ox_sim.h) groups the calls by topic:

- Lifecycle: `ox_sim_initialize` and `ox_sim_shutdown`. Calling initialize more than once is safe.
- Status: session state, active flag, and FPS.
- Views: view count, metadata, and a copy of the latest RGBA8 pixels for one eye.
- Profile: get or set the profile ID and read display metadata.
- Devices: list device info and get or set a live pose and active state.
- Components: list the inputs on a device.
- Inputs: typed get and set for boolean, float, and vec2 values.

Most calls return an `OxSimResult`: success, invalid argument, not initialized, buffer too small, profile not found, device not found, or component not found. Output strings use caller-owned buffers and are null-terminated. The API is plain C with `OX_DRIVER_EXPORT` visibility, which makes it easy to bind from Python or another language.

### Lifecycle and threads

The simulator state belongs to the process. In single-process mode, the application, the runtime, and the test share it. In split-process mode, it lives in the `ox` host process, and the GUI and REST API are the way to reach it.

`ox_sim_initialize` increases a reference count. `ox_sim_shutdown` decreases it and releases the state when it reaches zero. The driver initializes on load and shuts down on unload. If your test calls `ox_sim_initialize`, call `ox_sim_shutdown` to balance it.

## Controlling the simulator

The simulator can be controlled from a GUI (`src/gui`), a REST API (`src/rest_api`), a C API (`include/ox_sim.h`), or a language wrapper (`wrappers/`, mainly the Python `ox_sim.py`). The GUI and REST server both call the C API. A test using `ox_sim.h` and a person using the GUI change the same state.

### GUI

Run `ox` with the GUI enabled. The shipped `config.yaml` enables it. The window has a toolbar at the top, an eye preview on the left, and a device sidebar on the right.

The toolbar can register ox as the OpenXR runtime, copy the runtime manifest path, start and stop the REST server on port `8765`, copy the API URL, and switch the simulated device profile.

The preview shows the latest eye image the application submitted. A selector chooses the left eye, the right eye, or both side by side. The top-left shows whether a session is active. A copy button saves the current image to the clipboard.

Click the preview to give it focus, then move the head with the keyboard and mouse:

| Input | Action |
|---|---|
| Left-drag | Look around: yaw and pitch. No roll. |
| `W` / `S` | Move forward or backward along the current yaw. |
| `A` / `D` | Strafe left or right relative to yaw. |
| `R` / `F` | Move up or down along world up. |
| `Shift` | Move at 3x speed. |
| Arrow keys | Pitch and yaw, same as mouse-look. |
| `Q` / `E` | Roll around the view direction. |

Each device panel has an **Active** toggle, X/Y/Z position fields, pitch/yaw/roll fields in degrees, and a **Reset Pose** button. Boolean inputs are toggles. Float inputs are sliders. Trigger and squeeze sliders also have a hold button that ramps to `1.0` in 300 ms and back to `0.0` in 100 ms.

The full GUI reference is in the [GUI guide](../docs/gui.md).

### REST API

Start the GUI, turn on **API Server**, and send HTTP requests to `http://127.0.0.1:8765`. The server listens on the local machine only.

| Method and path | Purpose |
|---|---|
| `GET /v1/status` | Session state and application FPS. |
| `GET /v1/profile` | Active profile and its devices and inputs. |
| `PUT /v1/profile` | Switch profile with `{"profile_id": "htc_vive"}`. |
| `GET /v1/devices/<user_path>` | Read pose and active state. |
| `PUT /v1/devices/<user_path>` | Set position, orientation, and active state. |
| `GET /v1/inputs/<binding_path>` | Read an input value. |
| `PUT /v1/inputs/<binding_path>` | Set an input value. |
| `GET /v1/views/<eye>` | Eye image as PNG. Optional `?size=128`. |

URL paths leave out the leading slash:

```http
GET /v1/devices/user/hand/left
PUT /v1/inputs/user/hand/right/input/trigger/value
```

A device update body looks like:

```json
{
  "position": {"x": -0.2, "y": 1.4, "z": -0.3},
  "orientation": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0},
  "active": true
}
```

An input update must match the component type. Use `{"value": 0.8}` for a float, `{"value": true}` for a boolean, or `{"x": 0.25, "y": -0.5}` for a vector. The [REST API guide](../docs/rest_api.md) lists the response codes.

### C API

The C API is for tests and language bindings. Include [`ox_sim.h`](https://github.com/ox-runtime/ox-sim-driver/blob/main/include/ox_sim.h), call `ox_sim_initialize()`, and shut down when you are done. The runtime and simulator must already be selected through the environment.

```c
#include "ox_sim.h"

ox_sim_initialize();
ox_sim_set_profile("oculus_quest_3");

OxDeviceState headset = {};
headset.pose.position = {10.0f, 1.6f, 0.0f};
headset.pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
headset.is_active = XR_TRUE;
ox_sim_set_device("/user/head", &headset);

ox_sim_set_input_float("/user/hand/right", "/input/trigger/value", 0.85f);

OxDeviceState state = {};
ox_sim_get_device("/user/hand/right", &state);
ox_sim_shutdown();
```

The API also handles profile metadata, device and input listing, status, and eye images. To read an eye image, call `ox_sim_get_view_info` for the buffer size, allocate that many bytes, then call `ox_sim_get_view`. The pixels are RGBA8. The [C API reference](https://github.com/ox-runtime/ox-sim-driver/tree/main/include) lists every call and return code.

### Python wrapper

Download [`ox_sim.py`](https://github.com/ox-runtime/ox-sim-driver/blob/main/wrappers/python/ox_sim.py) and import `Simulator`. It calls the C API, so the runtime and simulator still run in the calling process.

```python
from ox_sim import Simulator

sim = Simulator()
sim.profile = "oculus_quest_3"

headset = sim.device("/user/head")
headset.position = (42.0, 1.6, 0.0)

right = sim.device("/user/hand/right")
right.orientation = (0.0, 0.0, 0.0, 1.0)
right.set_input("/input/trigger/value", 0.85)
right.set_input("/input/a/click", True)

pixels, width, height = sim.views()[0].image()  # RGBA8
sim.shutdown()
```

The wrapper raises errors such as `OxSimDeviceNotFoundError` and `OxSimComponentNotFoundError`. The [Python README](https://github.com/ox-runtime/ox-sim-driver/tree/main/wrappers/python) has the full list.

### Which one to use

| Situation | Use |
|---|---|
| You are developing by hand and want to see the eye image or tweak a pose | GUI |
| Another process or an agent needs to drive the app over HTTP | REST API |
| Automated tests inside the application, especially in CI | C API or Python wrapper |
| Blender tests or other Python test harnesses | Python wrapper |

For tests, use in-process control. It removes socket and shared-memory timing, which is why single-process mode exists. Use the REST API when you cannot run code in the application process, or when a person or agent needs to watch and steer a running session.

### Enabling the GUI and REST API

Both are optional and controlled by `config.yaml`:

```yaml
drivers:
  simulator:
    gui:
      enabled: true
    rest_api:
      enabled: true
      port: 8765
```

The simulator README and the C API README cover build and install.

## Related pages

- [Testing an application](testing.md)
- [Drivers](drivers.md) for the driver API the simulator implements.
- [Architecture](architecture.md) for where the simulator runs.
- [IPC proxy](ipc.md) for how the host reaches the application.
- [ox-sim-driver README](https://github.com/ox-runtime/ox-sim-driver#readme)
