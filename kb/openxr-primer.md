# OpenXR primer

OpenXR is the standard API between an XR application and the XR runtime on the machine. This page explains only the parts you need to read the rest of the KB. The [OpenXR specification](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html) has the full details.

## Loader, runtime, driver

An OpenXR application does not load the runtime itself. It links the OpenXR loader, a small library that finds and loads the active runtime. The runtime is usually supplied by a headset vendor or a platform owner.

```
Application
    -> OpenXR loader
        -> active runtime (a shared library)
            -> the runtime's device driver
```

The **loader** implements the public `xr*` functions and forwards them to the runtime. The **runtime** implements the OpenXR state machine, validation, swapchains, actions, and spaces. It also talks to the hardware, usually through a separate **driver**.

Splitting the work this way lets one application run against different runtimes. It also lets ox swap the simulator for a real driver without changing the application.

## How a runtime is chosen

The loader looks for a runtime manifest. The `XR_RUNTIME_JSON` environment variable overrides the search and points at a JSON file:

```json
{
  "file_format_version": "1.0.0",
  "runtime": {
    "name": "ox-runtime",
    "library_path": "./libox_runtime.so"
  }
}
```

The `library_path` tells the loader which shared library to load. This is why the ox guides start with:

```bash
export XR_RUNTIME_JSON=/path/to/ox/ox_runtime.json
```

Without that variable, the loader uses the machine's registered runtime. Setting `XR_RUNTIME_JSON` selects ox for one shell or one test run. The ox GUI can also register ox as the system default.

## Instances and systems

`xrCreateInstance` connects the application to the runtime. It passes application info and the requested API version. Most applications create one instance.

`xrGetSystem` asks for a system for a form factor, usually `XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY`. The runtime returns an `XrSystemId`. `xrGetSystemProperties` describes that system: name, vendor, maximum swapchain size, and tracking features. The driver supplies these values.

## Sessions

A **session** is one application's use of the XR system. `xrCreateSession` creates it. The application usually passes a graphics binding in the `next` chain, which tells the runtime which graphics API and device it will use.

The application calls `xrPollEvent` and looks for `XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED`. The states you need to know:

| State | Meaning |
|---|---|
| `IDLE` | The session exists but is not running. |
| `READY` | The application should call `xrBeginSession`. |
| `SYNCHRONIZED` | The session is running but may not be visible. |
| `VISIBLE` | The session is visible but may not have focus. |
| `FOCUSED` | The session has focus. Render and read input normally. |
| `STOPPING` | Stop rendering and call `xrEndSession`. |
| `EXITING` | Destroy the session. |
| `LOST` | The session cannot continue. |

The exact order of states depends on the runtime. A test harness often runs in `FOCUSED` and ignores transitions it does not care about.

A typical frame is:

1. `xrWaitFrame` to get the predicted display time.
2. `xrBeginFrame`.
3. `xrLocateViews` to get eye poses and fields of view.
4. Render into the swapchain images.
5. `xrEndFrame` with one or more layers.

## Actions

OpenXR does not hand applications raw button events. Instead, the application declares **actions** such as "grab" or "shoot", and groups them into **action sets**. It suggests bindings from those actions to device paths, attaches the action sets to the session, and calls `xrSyncActions` once per frame.

The runtime resolves the bindings. The application then reads action state with `xrGetActionStateBoolean`, `xrGetActionStateFloat`, `xrGetActionStateVector2f`, or `xrGetActionStatePose`.

Paths look like this:

```
/user/hand/right/input/trigger/value
/user/hand/left/input/thumbstick
/user/head
```

The first part is the **user path** (`/user/hand/right`). The rest is the **component path** (`/input/trigger/value`). A driver reports values for those paths. ox uses the same split in its driver API and simulator API.

An **interaction profile** names the bindings for a kind of controller, such as `/interaction_profiles/oculus/touch_controller`. The runtime reports the active profile with `xrGetCurrentInteractionProfile`.

## Swapchains

A **swapchain** is a set of images the application renders into. The usual steps are:

1. `xrEnumerateSwapchainFormats` to choose a format.
2. `xrCreateSwapchain` with the size, format, and image count.
3. `xrEnumerateSwapchainImages` to get the graphics API handles (GL texture IDs, Vulkan images, or Metal textures).
4. Each frame: `xrAcquireSwapchainImage`, render, then `xrReleaseSwapchainImage`.
5. Pass the swapchain image in a layer to `xrEndFrame`.

The runtime displays or consumes the images. ox copies the submitted image to CPU memory and gives it to the driver. That is how the simulator shows a preview and how tests read pixels.

## Reference spaces

A pose is always relative to a **space**. The common reference space types are:

| Type | Meaning |
|---|---|
| `VIEW` | Relative to the viewer's current pose. |
| `LOCAL` | An application-defined origin, often recentered at session start. |
| `STAGE` | A floor-relative space with a known forward direction. |
| `LOCAL_FLOOR` | Floor-relative, with no guaranteed stage origin. |

`xrCreateReferenceSpace` creates a space from an `XrReferenceSpaceCreateInfo`. That struct includes a pose that maps the application's origin into the space. `xrLocateSpace` returns the pose of one space inside another. `xrLocateViews` returns eye poses in a requested space.

Applications often create action spaces with `xrCreateActionSpace`, so a controller pose is relative to a chosen action. Tests usually set device poses in the same coordinates the application sees.

## Extensions

Features beyond the core spec are exposed as extensions. The application enumerates them with `xrEnumerateInstanceExtensionProperties` and enables them at instance or session creation. Graphics requirement functions, such as `xrGetOpenGLGraphicsRequirementsKHR`, are extensions too. ox implements a subset. See [runtime](runtime.md) for what it handles.

## Further reading

- [OpenXR specification](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html), especially the chapters on the loader, sessions, actions, spaces, and swapchains.
- [OpenXR loader design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md).
- [Architecture](architecture.md) for how ox splits these ideas across processes.
