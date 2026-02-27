# Run using `blender --python test_blender_draw_handler.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_openxr.json" before running blender

import bpy

frames = 0


def test_xr_with_draw_handler():
    def draw_callback():
        global frames
        frames += 1
        if frames > 10:
            print("Test passed: Draw handler executed in XR session")
            exit(0)

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    # Get the 3D View from the default screen
    screen = bpy.context.screen
    area = next((a for a in screen.areas if a.type == "VIEW_3D"), None)

    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()


# Run the test
test_xr_with_draw_handler()
