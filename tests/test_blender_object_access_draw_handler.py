# Run using `blender --python test_blender_timer_with_draw_handler.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_openxr.json" before running blender

import bpy
from mathutils import Vector


def test_object_access_in_xr():
    def draw_callback():
        bpy.data.objects["Light"].location = Vector((1, 1, 1))
        new_loc = bpy.data.objects["Light"].location
        print(f"Light location in draw handler: {new_loc}")
        assert (
            abs(new_loc.x - 1) < 0.01 and abs(new_loc.y - 1) < 0.01 and abs(new_loc.z - 1) < 0.01
        ), "Light location was not updated correctly in draw handler"
        print("Test passed: Light location updated correctly in draw handler")
        exit(0)

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    # Get the 3D View from the default screen and start XR session
    screen = bpy.context.screen
    area = next((a for a in screen.areas if a.type == "VIEW_3D"), None)

    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()


# Run the test
test_object_access_in_xr()
