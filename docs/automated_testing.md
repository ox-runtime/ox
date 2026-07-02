# Automated Testing

Test your OpenXR application with a virtual XR device, using your existing test framework.

## Common Setup
1. Download [ox](https://github.com/ox-runtime/ox/releases).
2. Set the `XR_RUNTIME_JSON` environment variable to point to ox:

    * **Windows:** `set XR_RUNTIME_JSON=C:\path\to\ox\ox_openxr.json`
    * **Linux/macOS:** `export XR_RUNTIME_JSON=/path/to/ox/ox_openxr.json`

3. Set `OX_USE_SIMULATOR` to enable the virtual XR device:

    * **Windows:** `set OX_USE_SIMULATOR=1`
    * **Linux/macOS:** `export OX_USE_SIMULATOR=1`

In this configuration, the ox runtime and simulator will run inside your application process. This helps keep your tests focused on your application logic, without the unnecessary variability of inter-process communication (IPC).

## Pick the Language
* [Testing with Python](automated_testing.md#testing-with-python)
* [Testing with C](automated_testing.md#testing-with-c)

## Testing with Python
You can write your tests using any testing framework for Python (for e.g. pytest), and run them in the usual way (for e.g. `python -m pytest`).

Here's a quick start guide. Please check the [documentation](https://github.com/ox-runtime/ox-sim-driver/tree/main/wrappers/python) for a complete reference to `ox_sim.py`.

1. Download [ox_sim.py](https://github.com/ox-runtime/ox-sim-driver/blob/main/wrappers/python/ox_sim.py) into your project.
2. Create an instance of the `Simulator` class at a global level, and specify the XR hardware that it should simulate (for e.g. Quest 3):
```py
sim = Simulator()
sim.profile = "oculus_quest_3"  # you can change this at any point
```

3. Get a reference to a virtual XR device (for e.g. the headset and right controller):
```py
headset = sim.device('/user/headset')
right_controller = sim.device('/user/hand/right')
```

4. Set the position and orientation of the devices (in OpenXR's [frame-of-reference](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#fundamentals-coordinate-system)):
```py
headset.position = (42.0, 1.6, 0.0)  # (x, y, z)
right_controller.orientation = (1.0, 0.0, 0.0, 0.0)  # identity quaternion (w, x, y, z)
```

5. Set button states:
```py
right_controller.set_input("/input/trigger/value", 0.85)  # press the trigger 85%
right_controller.set_input("/input/a/click", True)  # press the A button
```

6. Get the device pose:
```py
headset_position = headset.position
right_controller_orientation = right_controller.orientation
```

7. Get the pixels being rendered to the headset:
```py
pixels, w, h = sim.views()[0].image()  # eye 0
x, y = (30, 40)  # for e.g. read (30, 40)
pixel_base = (y * w + x) * 4  # 4 bytes per pixel (RGBA)
print("value of pixel[30, 40] is ", pixels[pixel_base:pixel_base+4])
```

### Example

Here's an example snippet from [blender-xr-regression-tests](https://github.com/cmdr2/blender-xr-regression-tests), which uses ox to automatically test Blender's XR mode.
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

## Testing with C
You can write your tests using any testing framework for C, and compile and run them in the usual way.

Here's a quick start guide. Please check the [documentation](https://github.com/ox-runtime/ox-sim-driver/tree/main/include) for a complete reference to `ox_sim.h`.

1. Download [ox_sim.h](https://github.com/ox-runtime/ox-sim-driver/blob/main/include/ox_sim.h) into your project.

2. Initialize the simulator before running any tests, and shut it down when finished:
```c
#include "ox_sim.h"

ox_sim_initialize();
// ... your tests ...
ox_sim_shutdown();
```

3. Set the XR hardware profile to simulate (for e.g. Quest 3):
```c
ox_sim_set_profile("oculus_quest_3");  // you can change this at any point
```

4. Set the position and orientation of a virtual XR device (for e.g. the headset and right controller):
```c
OxDeviceState headset_state = {};
headset_state.pose = {{0.0f, 0.0f, 0.0f, 1.0f}, {42.0f, 1.6f, 0.0f}};  // orientation (x,y,z,w), position (x,y,z)
headset_state.is_active = XR_TRUE;
ox_sim_set_device("/user/head", &headset_state);

OxDeviceState right_state = {};
right_state.pose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
right_state.is_active = XR_TRUE;
ox_sim_set_device("/user/hand/right", &right_state);
```

5. Set button and input states:
```c
ox_sim_set_input_float("/user/hand/right", "/input/trigger/value", 0.85f);  // press the trigger 85%
ox_sim_set_input_boolean("/user/hand/right", "/input/a/click", XR_TRUE);    // press the A button
```

6. Get the device pose:
```c
OxDeviceState state = {};
ox_sim_get_device("/user/hand/right", &state);
XrVector3f position = state.pose.position;
XrQuaternionf orientation = state.pose.orientation;
```

7. Get the pixels being rendered to the headset:
```c
OxSimViewInfo view_info = {};
ox_sim_get_view_info(0, &view_info);  // eye 0

uint8_t* pixels = malloc(view_info.data_size);
ox_sim_get_view(0, pixels, view_info.data_size);

uint32_t x = 30, y = 40;
uint32_t pixel_base = (y * view_info.width + x) * 4;  // 4 bytes per pixel (RGBA)
printf("value of pixel[30, 40] is (%d, %d, %d, %d)\n",
    pixels[pixel_base], pixels[pixel_base+1], pixels[pixel_base+2], pixels[pixel_base+3]);

free(pixels);
```

### Example

Here's the same flow as the Python example above, written in C:
```c
#include "ox_sim.h"
#include <assert.h>

void test_foo() {
    ox_sim_set_profile("oculus_quest_3");

    // Move the headset
    OxDeviceState headset_state = {};
    headset_state.pose.position = {10.0f, 20.0f, 30.0f};
    headset_state.is_active = XR_TRUE;
    ox_sim_set_device("/user/head", &headset_state);

    // Move and rotate the right controller
    OxDeviceState right_state = {};
    right_state.pose.position = {42.0f, 42.0f, 42.0f};
    right_state.pose.orientation = {0.0f, 0.2588f, 0.0f, 0.9659f};  // ~30 degrees around Y
    right_state.is_active = XR_TRUE;
    ox_sim_set_device("/user/hand/right", &right_state);

    // Press the trigger 85%
    ox_sim_set_input_float("/user/hand/right", "/input/trigger/value", 0.85f);

    // ... wait for next frame, then verify your application state ...

    OxDeviceState verify_state = {};
    ox_sim_get_device("/user/hand/right", &verify_state);
    assert(verify_state.pose.position.x == 42.0f);
    assert(verify_state.pose.position.y == 42.0f);
    assert(verify_state.pose.position.z == 42.0f);
}
```
