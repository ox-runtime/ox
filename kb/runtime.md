# Runtime internals

`ox-runtime` is the shared library that the OpenXR loader loads. It implements the core OpenXR functions. For anything hardware-related, it calls a driver through `ox_driver.h`. Most of the code is in one file, [`src/runtime.cpp`](https://github.com/ox-runtime/ox-runtime/blob/main/src/runtime.cpp). The graphics code is split by API into separate headers.

## What the runtime does

The runtime handles:

- The handshake with the loader and the function table used by `xrGetInstanceProcAddr`.
- Instances, systems, sessions, and session-state events.
- Paths, reference spaces, and action spaces.
- Action sets, actions, bindings, and action state reads.
- Swapchain creation, image enumeration, and acquire/wait/release.
- Frame timing and the `xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`, and `xrLocateViews` calls.

The runtime does not know how a real device works. It asks the driver for system properties, eye views, device poses, input values, interaction profiles, and frame pixels.

## Loading a driver

`xrNegotiateLoaderRuntimeInterface` checks the loader and runtime interface structs. It then calls `LoadConfiguredDriver()` if no driver is installed yet. It returns the interface version, the API version, and `xrGetInstanceProcAddr`.

There are four ways to load a driver, in this order: a driver installed by `ox_set_driver`, the `OX_RUNTIME_DRIVER` path, `OX_USE_SIMULATOR=1` for `drivers/simulator/ox_driver`, and finally `ox_ipc_client`. See [architecture](architecture.md) for the details.

`ox_set_driver()` checks the required callbacks, calls the driver's `initialize()`, shuts down any old driver, and stores a copy of the callback table. Passing `nullptr` unloads the driver. This function exists for the runtime tests. It is not part of OpenXR.

## Sessions

`xrCreateSession` looks through the `next` chain for an OpenGL, Vulkan, or Metal graphics binding. It stores the binding in the session and starts the session in `XR_SESSION_STATE_READY`.

State changes are put on a queue for `xrPollEvent`. They are also sent to the driver through `on_session_state_changed`, which lets the simulator report session state to the GUI and REST API.

`xrBeginSession` moves the session to `FOCUSED`. `xrEndSession` returns it to `IDLE`. `xrRequestExitSession` moves it to `EXITING`.

`xrLocateViews` calls the driver's `update_view` for each eye and returns the pose and field of view. For an action space, `xrLocateSpace` calls `update_devices`, finds the matching user path, and returns that device's pose. A `VIEW` reference space returns the midpoint of the two eye poses. Other reference spaces currently return an identity pose.

## Actions and input

The runtime stores action sets, actions, and suggested bindings. `xrAttachSessionActionSets` picks the active interaction profile from the driver's `get_interaction_profiles` callback. `xrSyncActions` checks the call and returns success. Action values are read from the driver when the application asks for them.

To read an action, the runtime turns each binding into a user path and a component path. It checks that the binding belongs to the active profile and the requested subaction path, then calls the driver's typed input callback.

If more than one binding is active, the runtime combines them:

- Booleans are OR-ed together.
- Floats use the value with the largest absolute value.
- Vectors use the longer vector.

A float action can also fall back to a boolean input. That turns a button press into `0.0` or `1.0`.

The runtime implements a subset of the spec and does little validation. When you add a feature, check the spec and the conformance test suite instead of assuming the existing behavior is correct.

## Graphics backends

Each graphics API has a header in `src/`. They all do the same jobs: detect the session binding, create swapchain images, fill in the `XrSwapchainImage*` structs, copy a submitted image to CPU memory, and fix the pixel row order.

### OpenGL

[`graphics_opengl.hpp`](https://github.com/ox-runtime/ox-runtime/blob/main/src/graphics_opengl.hpp) detects Win32, Xlib, XCB, and Wayland OpenGL bindings. It creates GL textures, returns their IDs as `XrSwapchainImageOpenGLKHR`, and reads pixels with `glGetTexImage`. It reports `GL_SRGB8_ALPHA8` so applications handle sRGB correctly. `NormalizeFramePixels` flips the image, because OpenGL starts at the bottom-left.

### Vulkan

[`graphics_vulkan.hpp`](https://github.com/ox-runtime/ox-runtime/blob/main/src/graphics_vulkan.hpp) creates `VkImage` objects, finds device-local memory, and binds it. It returns the images as `XrSwapchainImageVulkanKHR`. Copying to CPU uses a staging buffer and a command buffer. The helper keeps track of the device, physical device, queue, command pool, and format.

### Metal

[`graphics_metal.hpp`](https://github.com/ox-runtime/ox-runtime/blob/main/src/graphics_metal.hpp) is Objective-C++ and only builds on Apple. It creates `MTLTexture` objects from an `MTLCommandQueue`, maps OpenXR format values to `MTLPixelFormat`, and copies texture data through a shared staging buffer.

The backend is compiled with `OX_OPENGL`, `OX_VULKAN`, or `OX_METAL`. `xrCreateSession` tries each enabled backend. Swapchain functions dispatch based on the session's graphics API.

## Submitting a frame

`xrEndFrame` currently handles projection layers only. For each view, it finds the swapchain, copies image 0 to a CPU buffer, normalizes the rows, and calls the driver's `submit_frame_pixels` with the display time, eye index, size, format, and RGBA data. The driver must not free that buffer. It stays valid until the next submission.

The runtime also estimates the display period from the time between `xrEndFrame` calls. It ignores intervals below 1 ms or above 500 ms. `xrWaitFrame` uses that estimate.

## Tests

The runtime tests are in [`tests/runtime`](https://github.com/ox-runtime/ox-runtime/tree/main/tests/runtime). They do not use the `ox` host or the IPC proxy. Instead, `common.h` defines `FakeDriverState`, a driver made of static callbacks over in-memory test data. The test fixture installs it with `ox_set_driver()` in `SetUp()`.

That gives each test a fake system, fake eye poses, fake devices, fake input maps, and a record of submitted frames. Tests can then cover instances, sessions, actions, swapchains, and frame handling without a GPU or a real driver.

Build them with:

```bash
cmake --build build --target runtime_tests --config Release
```

## Related pages

- [Architecture](architecture.md)
- [Drivers](drivers.md)
- [IPC proxy](ipc.md)
- [OpenXR primer](openxr-primer.md)
- [ox-runtime README](https://github.com/ox-runtime/ox-runtime#readme)
