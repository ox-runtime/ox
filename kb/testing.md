# Testing an application

This page is about running an OpenXR application against ox in a test and checking what the application does. It also covers the Blender XR regression suite, which is the largest public example.

The API calls used to control the simulator are in [Simulator](simulator.md).

## Setup

1. Get an ox release or build one from source.
2. Point `XR_RUNTIME_JSON` at the ox manifest:
   - Windows: `set XR_RUNTIME_JSON=C:\path\to\ox\ox_runtime.json`
   - Linux/macOS: `export XR_RUNTIME_JSON=/path/to/ox/ox_runtime.json`

   The build creates `ox_runtime.json`. Some docs and older releases call it `ox_openxr.json`. Use the file that exists in your build or release.

3. Turn on the simulator:
   - Windows: `set OX_USE_SIMULATOR=1`
   - Linux/macOS: `export OX_USE_SIMULATOR=1`

With those two variables, the runtime and simulator run inside the application process. There is no `ox` host and no IPC. Test code can call the `ox_sim_*` functions directly.

That matters for reliability. Split-process mode adds a socket, shared memory, and a frame-forwarding thread. All three can add timing differences. Use single-process mode unless the test is about the host or the IPC layer.

## How a test works

The test process and the application process are the same process. The test sets device state, lets the application run a frame, then checks the application's own state.

A short Python test using `ox_sim.py`. The helper functions stand in for your application's frame loop and state access:

```python
from ox_sim import Simulator

sim = Simulator()
sim.profile = "oculus_quest_3"


def test_base_pose():
    headset = sim.device("/user/head")
    headset.position = (42.0, 1.6, 0.0)

    wait_for_frame()  # let the application run one frame

    # Read the application's state, not just the simulator's.
    assert app_viewer_position() == (42.0, 1.6, 0.0)
```

The calls that set poses, set inputs, and read device state are listed in [Simulator](simulator.md), with examples in Python and C.

## Waiting for frames

Most applications use simulator changes on their next frame, not immediately. A test that sets a pose and reads application state in the same call may read stale data.

How to wait depends on the test harness:

- In Blender's regression harness, use `yield` inside a test function. The harness resumes the test on the next frame.
- In an application test, wait for the application's frame loop or a frame counter.
- In a standalone test, poll `sim.status` or the application state with a timeout.

Avoid fixed sleeps when a frame signal is available. They make tests slower and less reliable.

## Reading eye images

The runtime copies each submitted eye image to CPU memory. In a test, this is how you check what the application rendered:

```python
pixels, width, height = sim.views()[0].image()
offset = (40 * width + 30) * 4
rgba = pixels[offset:offset + 4]
```

Wait for at least one frame before reading. `sim.views()` returns one view per eye, and tracker-only profiles have none. See [Simulator](simulator.md) for the Python wrapper and C API details.

## Common mistakes

- Forgetting `OX_USE_SIMULATOR=1`. The runtime then tries to load `ox_ipc_client` and waits for a host that is not running.
- Forgetting `XR_RUNTIME_JSON`. The loader then uses a different runtime.
- Mixing Debug and Release builds of the runtime and driver.
- Reading an eye image before the application has submitted a frame.
- Using host-mode paths or REST calls in a test that expects single-process behavior.

## Blender XR regression tests

The [`blender-xr-regression-tests`](https://github.com/cmdr2/blender-xr-regression-tests) suite uses ox to test Blender's XR API automatically. It is an unofficial project. It is not affiliated with the Blender project.

The suite drives Blender's XR API with a virtual device and checks the results. It is useful during Blender development and in CI. The project notes that it should eventually move into Blender's own `tests/python` folder. See [`test_xr_session_state.py`](https://github.com/cmdr2/blender-xr-regression-tests/blob/main/test_xr_session_state.py) for a complete example.

### How it runs

The ox runtime loads inside the running Blender process. There is no `ox` host, no IPC, and no HTTP server. The suite sets `OX_USE_SIMULATOR=1` and points `XR_RUNTIME_JSON` at ox, so Blender's OpenXR session uses the simulator directly. Test code in Blender's Python environment calls `ox_sim.py`, which reaches the same simulator state.

Set up `XR_RUNTIME_JSON` and `OX_USE_SIMULATOR` as in [Setup](#setup), then run:

```sh
/path/to/blender --python /path/to/harness.py
```

A Blender window opens. Results print to the console, and progress appears in the status bar.

The suite includes its own `actions_helper` package for XR actions. It does not need an external input add-on.

### Filtering tests

Arguments after `--` go to the harness:

```sh
# Run everything
blender --python harness.py --

# Only these files, matched by filename
blender --python harness.py -- test_xr_session_state.py test_xr_actions.py

# Only functions whose name contains a string
blender --python harness.py -- -k base_pose_type

# Both
blender --python harness.py -- test_xr_session_settings.py -k base_pose_type
```

`-k` matches one plain substring. It does not support `and`, `or`, or expressions.

### What it covers

| Area | Status |
|---|---|
| Session start, stop, restart | Covered |
| Draw handlers and app timers in XR | Covered |
| Base pose tracking (camera, object, custom) | Covered |
| Headset and controller pose tracking | Covered |
| Controller buttons, trigger, squeeze, joystick, touch | Covered |
| Viewport clip distances | Not covered |
| Controller draw style | Not covered |
| Visibility flags (camera, light, floor, handles) | Not covered |
| Passthrough | Not covered |
| VR scene inspector operators (Fly, Teleport, Strafe) | Not covered |

### How the harness works

`harness.py` works like a small pytest. It finds files starting with `test_`, then functions starting with `test_` in each file. It supports `setup_module`, `teardown_module`, `setup_function`, and `teardown_function`.

Tests can run across frames. A test function can be a generator. A `yield` pauses it until the next Blender frame. The harness runs each generator from an app timer, so Blender's main thread is never blocked. That is how a test can set a value in one frame and check the result in the next.

A small multi-frame test:

```python
def test_pose_update():
    settings = bpy.context.window_manager.xr_session_settings
    state = bpy.context.window_manager.xr_session_state

    settings.base_pose_location = Vector((42, 42, 42))
    yield  # resume next frame
    assert vec_equal(state.viewer_pose_location, Vector((42, 42, 42)))
```

This suite tests ox as well as Blender. If it fails, first check that the runtime and driver use the same build configuration. Then work out whether Blender changed or ox did.

## Smoke tests

The pyopenxr examples are small OpenXR programs that double as smoke tests. `track_hmd2.py` prints headset poses and `track_controller.py` prints controller poses. They are a quick way to check that the runtime is loaded and the device is alive. See [Companion projects](companion-projects.md).

## Related pages

- [Simulator](simulator.md)
- [Architecture](architecture.md)
- [Automated testing guide](../docs/automated_testing.md)
- [blender-xr-regression-tests README](https://github.com/cmdr2/blender-xr-regression-tests#readme)
