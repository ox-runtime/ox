Setup instructions and example tests for Blender (automated XR testing).

**Tip:** You can also use the [GUI](./gui.md) version of ox (similar to Meta Simulator) for interactive testing of Blender's XR mode.

## Setup
Download [ox](https://github.com/ox-runtime/ox/releases).

Set the following environment variables:
```
export XR_RUNTIME_JSON="/path/to/ox/ox_runtime.json"
export OX_USE_SIMULATOR=1
```

**Note for Mac**: If you installed using a `.dmg`, then `ox_runtime.json` can be found at `/path/to/ox.app/Contents/Resources/ox_runtime.json`

Run the following tests with `./blender --python /path/to/test.py`

## test_xr_start.py
```py
import bpy
import traceback

def test_start_xr():
    area = next((a for a in bpy.context.screen.areas if a.type == "VIEW_3D"), None)
    wm = bpy.context.window_manager

    assert wm.xr_session_state is None

    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()

    assert wm.xr_session_state is not None
    assert wm.xr_session_state.is_running(bpy.context)

    print("Test passed: Draw handler executed in XR session")
    bpy.ops.wm.quit_blender()

if __name__ == "__main__":
    try:
        test_start_xr()
    except:
        traceback.print_exc()
```

## test_xr_draw_handler.py
```py
import bpy
import traceback

frames = 0

def test_draw_handler_xr():
    def draw_callback():
        global frames
        frames += 1
        if frames > 10:
            print("Test passed: Draw handler executed in XR session")
            bpy.ops.wm.quit_blender()

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    area = next((a for a in bpy.context.screen.areas if a.type == "VIEW_3D"), None)
    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()

if __name__ == "__main__":
    try:
        test_draw_handler_xr()
    except:
        traceback.print_exc()
```

## test_xr_object_access_in_draw_handler.py
```py
import bpy
import traceback
from mathutils import Vector

eps = 0.001

def test_draw_handler_xr():
    def draw_callback():
        bpy.data.objects["Light"].location = Vector((1, 1, 1))
        new_loc = bpy.data.objects["Light"].location
        print(f"Light location in draw handler: {new_loc}")

        assert abs(new_loc.x - 1) < eps
        assert abs(new_loc.y - 1) < eps
        assert abs(new_loc.z - 1) < eps

        print("Test passed: Light location updated correctly in draw handler")
        bpy.ops.wm.quit_blender()

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    area = next((a for a in bpy.context.screen.areas if a.type == "VIEW_3D"), None)
    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()

if __name__ == "__main__":
    try:
        test_draw_handler_xr()
    except:
        traceback.print_exc()
```
