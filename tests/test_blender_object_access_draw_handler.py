# Run using `blender --python test_blender_timer_with_draw_handler.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_openxr.json" before running blender

import bpy


def test_object_access_in_xr():
    # Define a simple draw handler
    def draw_callback(context):
        # This handler runs during VR rendering
        print("Draw handler executed in XR session")
        print("Context in draw handler:", context)

        bpy.data.objects["Light"].location.x += 0.01

    # Add the draw handler to the 3D View
    print("Context before registering draw handler:", bpy.context)
    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (bpy.context,), "XR", "POST_VIEW")

    # Get the 3D View from the default screen and start XR session
    screen = bpy.context.screen
    area = next((a for a in screen.areas if a.type == "VIEW_3D"), None)

    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()


# Run the test
test_object_access_in_xr()
