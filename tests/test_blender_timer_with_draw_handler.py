# Run using `blender --python test_blender_timer_with_draw_handler.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_openxr.json" before running blender

import bpy

TIMER_INTERVAL = 0.5  # seconds


def test_timers_in_xr():
    # Define a timer
    def _test_timer():
        print("Timer check in XR session")

        print("Context", bpy.context)
        view_layer = bpy.context.view_layer
        print(f"View layer accessed successfully: {view_layer}")

        return TIMER_INTERVAL

    bpy.app.timers.register(_test_timer, persistent=True)

    # Define a simple draw handler
    def draw_callback():
        print("Draw handler executed in XR session")

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    # Get the 3D View from the default screen and start XR session
    screen = bpy.context.screen
    area = next((a for a in screen.areas if a.type == "VIEW_3D"), None)

    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()


# Run the test
test_timers_in_xr()
