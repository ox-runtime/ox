# Regression tests for Blender's XR mode

## Install
1. Download or clone this repository.
2. Download and install [ox](https://github.com/ox-runtime/ox/releases).

`ox` is an open-source OpenXR runtime that supports programmatic control of virtual XR devices, and is focused on automated headless testing of OpenXR apps (on Windows/Linux/Mac across OpenGL/Vulkan/Metal).

## Run the Tests
1. `export XR_RUNTIME_JSON=/path/to/ox/ox_runtime.json`
2. `export OX_USE_SIMULATOR=1`
3. Run the tests using `/path/to/blender --python /path/to/harness.py`

## Writing tests that span across frames
It's really easy to write a test that spans across multiple frames. Just use a `yield` statement in your test, and the harness will wait until the next frame to run the code following that `yield`. You can `yield` multiple times.

Here's a short example that sets a value in one frame, and verifies it in the next frame:
```python
def test_foo():
    settings = bpy.context.window_manager.xr_session_settings
    state = bpy.context.window_manager.xr_session_state

    settings.base_pose_location = Vector((42, 42, 42))

    yield  # resumes in the next frame

    assert (state.viewer_pose_location - Vector((42, 42, 42))).length < 0.001
```

## Controlling virtual devices
These tests use [ox_sim.py](https://github.com/ox-runtime/ox-sim-driver/tree/main/wrappers/python) to connect to the virtual XR device in `ox`. `ox_sim.py` is a Pythonic wrapper around the [ox simulator's C-API](https://github.com/ox-runtime/ox-sim-driver/blob/main/include/ox_sim.h).

See [test_xr_session_state.py](test_xr_session_state.py#L171) for a complete example that uses `ox_sim.py` in a test.

Here's a short example that sets the position of the headset and a controller:
```python
def test_foo():
   headset = sim.device("/user/head")
   headset.position = Vector((10, 20, 30))  # move the headset

   right_controller = sim.device("/user/hand/right")
   right_controller.orientation = Quaternion((0, 1, 0), radians(30))  # rotate the controller

   right_controller.set_input("/input/trigger/value", 0.85)  # press the trigger 85%

   yield  # until the next frame

   # assert the value of `bpy.context.window_manager.xr_session_state.controller_grip_location_get(bpy.context, 1)`
```


## Defining tests
`harness.py` behaves like pytest.

It will look for files starting with `test_`, and then look for functions starting with `test_` in each file.

### Fixtures
You can optionally define the following functions in each test file:
- `setup_module()`: Called once before all tests in that test file.
- `teardown_module()`: Called once after all tests in that test file.
- `setup_function()`: Called before each test function.
- `teardown_function()`: Called after each test function.

## How does it work across frames?
`harness.py` runs tests across multiple Blender frames to ensure that the XR state is applied properly.

This is achieved by using app timers, and running each test function as a generator (which yields after each frame).

Each test function's generator is called once per timer callback, until the generator completes or raises an exception. This way, each test can yield and continue in the next timer callback, allowing it to run across multiple frames.
