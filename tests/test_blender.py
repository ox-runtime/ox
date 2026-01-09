# Run using `blender --python test_blender.py`

import bpy

# Get the 3D View from the default screen
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
