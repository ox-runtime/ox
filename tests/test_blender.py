# Run using `blender --python test_blender.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_runtime.json" before running blender

import bpy

# Get the 3D View from the default screen
screen = bpy.context.screen
area = next((a for a in screen.areas if a.type == "VIEW_3D"), None)

bpy.context.window_manager.xr_session_settings.base_scale = 10

with bpy.context.temp_override(area=area):
    bpy.ops.wm.xr_session_toggle()
