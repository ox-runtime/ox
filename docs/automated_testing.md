# Automated Testing

Test your OpenXR application with a virtual XR device, using your existing testing framework.

In this configuration, the ox runtime will run inside your application process. This lets you to keep your tests focused on the application logic, without including the additional complexity of cross-process communication in your test path.

## Setup
1. Download [ox](https://github.com/ox-runtime/ox/releases).
2. Get one of the wrappers to communicate with the virtual XR device in your tests:
- Python: [ox_sim.py](https://github.com/ox-runtime/ox-sim-driver/tree/main/wrappers/python)
- C: [ox_sim.h](https://github.com/ox-runtime/ox/blob/main/docs/c_api.md)

Please see the respective wrapper documentation for writing tests in that language.

## Run the Tests
1. Set the `XR_RUNTIME_JSON` environment variable to point to the runtime manifest:

* **Windows:** `set XR_RUNTIME_JSON=C:\path\to\ox\ox_openxr.json`
* **Linux/macOS:** `export XR_RUNTIME_JSON=/path/to/ox/ox_openxr.json`

2. Set `OX_USE_SIMULATOR=1` to enable the virtual XR device:

* **Windows:** `set OX_USE_SIMULATOR=1`
* **Linux/macOS:** `export OX_USE_SIMULATOR=1`

3. Run your tests using your regular testing framework, e.g. `python -m pytest`.

## Example

Here's an example of how `ox` can be used for automatically testing [Blender's XR mode](https://github.com/cmdr2/blender-xr-regression-tests):

```python
def test_foo():
    state = bpy.context.window_manager.xr_session_state

    headset = sim.device("/user/head")
    headset.position = Vector((10, 20, 30))  # move the headset

    right_controller = sim.device("/user/hand/right")
    right_controller.position = Vector((42, 42, 42))
    right_controller.orientation = Quaternion((0, 1, 0), radians(30))  # rotate the controller

    right_controller.set_input("/input/trigger/value", 0.85)  # press the trigger 85%

    yield  # until the next frame

    loc = state.controller_grip_location_get(bpy.context, 1)
    expected_loc = openxr_to_blender_vec(right_controller.position)
    assert vec_equal(loc, expected_loc)
```
