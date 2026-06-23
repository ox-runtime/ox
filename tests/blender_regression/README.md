# Regression tests for Blender's XR mode

## Installation
1. Download or clone this repository.
2. Download and install [ox](https://github.com/ox-runtime/ox/releases) (an OpenXR runtime that supports programmatic control of XR devices).

## Running the Tests
1. Set the `XR_RUNTIME_JSON` environment variable to the path of the ox runtime JSON, e.g. `export XR_RUNTIME_JSON=/path/to/ox/runtime.json`.
2. Set the `OX_USE_SIMULATOR` environment variable to `1` to use the simulator, e.g. `export OX_USE_SIMULATOR=1`.
3. Run the tests using `/path/to/blender --python /path/to/harness.py`

## Test Structure
`harness.py` behaves like pytest, and will run all the test functions (starting with "test_") in files that start with "test_". E.g. "test_foo.py" containing "test_bar()".

You can also define the following functions in each test file:
- `setup_module()`: Called once before all tests in the module.
- `teardown_module()`: Called once after all tests in the module.
- `setup_function()`: Called before each test function.
- `teardown_function()`: Called after each test function.

This uses [ox_sim.py](https://github.com/ox-runtime/ox-sim-driver/blob/main/wrappers/python/ox_sim.py) to connect to the simulator device in [ox-runtime](https://github.com/ox-runtime/ox).

### How does it work across frames?
`harness.py` runs tests across multiple Blender frames to ensure that the XR state is applied properly.

This is achieved by running each test function as a generator (which yields after each frame).

Each test function's generator is called once per timer callback, until the generator completes or raises an exception. This way, each test can yield and continue in the next timer callback, allowing it to run across multiple frames.

## Writing tests that span across frames
It's really easy to write a test that spans across multiple frames. Just use a `yield` statement in your test, and harness will run the code after that in the next frame.

For e.g. you can set a value in one frame, and verify it in the next frame:
```python
def test_foo():
    settings = bpy.context.window_manager.xr_session_settings
    state = bpy.context.window_manager.xr_session_state

    settings.base_pose_location = Vector((42, 42, 42))

    yield  # resumes in the next frame

    assert (state.viewer_pose_location - Vector((42, 42, 42))).length < 0.001
```
