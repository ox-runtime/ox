# Run using `blender --python test_blender_restart.py`
# Remember to set XR_RUNTIME_JSON="/path/to/ox_openxr.json" before running blender

import bpy


def is_xr_running():
    """Check if XR session is running"""
    state = bpy.context.window_manager.xr_session_state
    return state is not None and state.is_running(bpy.context)


def wait_for_running_then_wait(area):
    """Wait for XR to start, then wait 1 second"""
    if is_xr_running():
        print("VR session started successfully")
        print("Waiting 1 second...")
        bpy.app.timers.register(lambda: stop_vr(area), first_interval=1.0)
    else:
        # Check again in 0.1 seconds
        bpy.app.timers.register(lambda: wait_for_running_then_wait(area), first_interval=0.1)


def stop_vr(area):
    """Stop the VR session"""
    print("Stopping VR session...")
    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()
    bpy.app.timers.register(lambda: wait_for_stopped_then_restart(area), first_interval=0.1)


def wait_for_stopped_then_restart(area):
    """Wait for XR to stop, then restart"""
    if not is_xr_running():
        print("VR session stopped")
        print("Restarting VR session...")
        with bpy.context.temp_override(area=area):
            bpy.ops.wm.xr_session_toggle()
        bpy.app.timers.register(lambda: wait_for_restart(area), first_interval=0.1)
    else:
        # Check again in 0.1 seconds
        bpy.app.timers.register(lambda: wait_for_stopped_then_restart(area), first_interval=0.1)


def wait_for_restart(area):
    """Wait for XR to restart"""
    if is_xr_running():
        print("VR session restarted successfully")
    else:
        # Check again in 0.1 seconds
        bpy.app.timers.register(lambda: wait_for_restart(area), first_interval=0.1)


# Get the 3D View from the default screen
screen = bpy.context.screen
area = next((a for a in screen.areas if a.type == "VIEW_3D"), None)

with bpy.context.temp_override(area=area):
    bpy.ops.wm.xr_session_toggle()

# Start the asynchronous sequence
bpy.app.timers.register(lambda: wait_for_running_then_wait(area), first_interval=0.1)
