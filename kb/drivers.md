# Drivers

A driver is a shared library that connects the ox runtime to a device. It implements the C API in [`ox_driver.h`](https://github.com/ox-runtime/ox-runtime/blob/main/include/ox_driver.h) and exports one function. The runtime loads the library, asks for the callback table, and calls those callbacks for system info, views, devices, inputs, and frames.

The simulator is also a driver. See [simulator](simulator.md).

## The API

A driver exports:

```c
int ox_driver_register(OxDriver* driver);
```

The runtime allocates the `OxDriver` struct. The driver fills in the function pointers. Return `1` on success and `0` on failure.

These callbacks are required:

| Callback | Purpose |
|---|---|
| `initialize` | Called once when the driver loads. Return `1` on success. |
| `is_device_connected` | Return non-zero when a usable device is present. |
| `get_system_properties` | Fill in the system name, vendor ID, swapchain limits, and tracking flags. |
| `update_view` | Return the pose and field of view for one eye at a predicted display time. |

These are optional, but most drivers need them:

| Callback | Purpose |
|---|---|
| `shutdown` | Release driver resources. |
| `is_driver_running` | Return `0` to tell the host to unload the driver. Defaults to running. |
| `set_config_bool` / `set_config_string` / `set_config_int` / `set_config_float` | Receive settings from `config.yaml`. |
| `update_devices` | Return controllers, trackers, and the headset pose. The headset should be device 0 with user path `/user/head`. |
| `get_input_state_bool` | Read a boolean input such as `/input/a/click`. |
| `get_input_state_float` | Read a float input such as `/input/trigger/value`. |
| `get_input_state_vector2f` | Read a vector input such as `/input/thumbstick`. |
| `get_interaction_profiles` | Return supported interaction profile paths. Defaults to `/interaction_profiles/khr/simple_controller`. |
| `on_session_state_changed` | Observe session state changes. |
| `submit_frame_pixels` | Receive the RGBA pixels of a submitted eye image. Do not free the buffer. |

`OX_MAX_DEVICES` is 16. `update_devices` writes at most that many states and reports the count. A device state has a user path, an `XrPosef`, and an active flag.

The driver decides what its input paths mean. The runtime only matches action bindings to user and component paths and calls the typed getter. A trigger binding becomes user path `/user/hand/right` and component path `/input/trigger/value`, and the driver reads whatever hardware it has.

## Folder layout and names

Drivers live in folders under the runtime's `drivers/` directory. The folder is named after the driver. The shared library inside must have the standard name for the platform:

```text
drivers/
└── my_vive_driver/
    └── ox_driver.dll
```

| Platform | File name |
|---|---|
| Windows | `ox_driver.dll` |
| Linux | `ox_driver.so` |
| macOS | `ox_driver.dylib` |

The runtime builds the library path from the folder name and lets the dynamic loader add the platform prefix and suffix. The host expects exactly one folder under `drivers/`. To test a custom driver in host mode, move the `simulator` folder out of the way.

## Writing a driver

Start from [`ox-driver-example`](https://github.com/ox-runtime/ox-driver-example). It reports a fixed-pose headset that is always connected, returns a standing pose at `(0, 1.6, 0)`, uses standard display properties, and receives frame pixels. That is enough to check that loading works before you add real hardware.

1. Clone the example and rename the project.
2. Include `ox_driver.h` and `openxr.h`. A driver needs only those headers. The example's CMake fetches them for you.
3. Implement the required callbacks. Start with a fixed pose and a connected device.
4. Build the shared library. The example writes `build/example_driver/`.
5. Copy the output folder into the runtime's `drivers/` folder.
6. Remove any other driver folder, because host mode allows only one.
7. Start `ox`, then run an OpenXR application with `XR_RUNTIME_JSON` pointing at the ox manifest.

The example README has the commands for each platform. To build against a local runtime checkout, pass `-DOX_RUNTIME_REPO=/path/to/ox-runtime` when you configure CMake. That uses local headers instead of fetching the remote repo.

## Debugging without IPC

The normal host path goes through IPC, which makes step-by-step debugging awkward. To skip it, set `OX_RUNTIME_DRIVER` to the driver library path:

```bash
export OX_RUNTIME_DRIVER=/path/to/ox/bin/drivers/my_vive_driver/ox_driver.so
```

The runtime turns the value into an absolute path and loads the driver into the application process. You do not need to run `ox`. If a relative path does not work, use a full path or run from the folder that contains the driver. This mode is for debugging. Normal users get the host and IPC.

## Simulator profiles versus drivers

If you want to simulate an existing headset in tests, do not write a driver. Add a profile to `ox-sim-driver/src/device_profiles.hpp`. The simulator already implements the driver API and gives you the GUI, REST API, C API, and Python wrapper for free.

Write a driver when you want to use real hardware, or when you want the runtime to see a new kind of device.

## Current limits

- One driver at a time. The host refuses to start if `drivers/` does not contain exactly one folder.
- No driver swap while a session is running.
- The API has a version number (`OX_DRIVER_API_VERSION`), but there is no negotiation beyond the current header.
- The runtime accepts haptics but does not forward them to the driver.
- `xrEndFrame` handles projection layers only, so a driver sees only those submitted pixels.

Treat the API as unstable while ox is before 1.0. Rebuild your driver against the runtime version you ship.

## Related pages

- [Runtime](runtime.md) for how the runtime calls the driver.
- [Architecture](architecture.md) for driver loading and process modes.
- [Simulator](simulator.md) for the reference driver.
- [Custom drivers guide](../docs/drivers.md)
- [ox-driver-example README](https://github.com/ox-runtime/ox-driver-example#readme)
