# Run using `blender --python test_blender_timer_with_draw_handler.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_openxr.json" before running blender

import bpy

MODE_CHECK_INTERVAL = 0.5  # seconds


def test_timers_in_xr():
    def _test_timer():
        print("Timer check in XR session")

        print("Context", bpy.context)
        view_layer = bpy.context.view_layer
        print(f"View layer accessed successfully: {view_layer}")

        return MODE_CHECK_INTERVAL

    # Define a simple draw handler
    def draw_callback():
        # This handler runs during VR rendering
        print("Draw handler executed in XR session")

    if not bpy.app.background:
        bpy.app.timers.register(_test_timer, persistent=True)

    # Add the draw handler to the 3D View
    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    # Get the 3D View from the default screen and start XR session
    screen = bpy.context.screen
    area = None
    for a in screen.areas:
        if a.type == "VIEW_3D":
            area = a
            break

    if area:
        with bpy.context.temp_override(area=area):
            bpy.ops.wm.xr_session_toggle()
    else:
        print("No 3D View found")


# Run the test
test_timers_in_xr()
