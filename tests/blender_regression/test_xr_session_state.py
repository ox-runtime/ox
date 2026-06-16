# setup harness
import sys
from pathlib import Path

script_dir = Path(__file__).parent.resolve()
sys.path.append(str(script_dir))

from harness import Harness

# /setup harness

import bpy

from math import radians
from mathutils import Vector, Quaternion

from common import (
    toggle_xr,
    blender_to_openxr_vec,
    blender_to_openxr_quat,
    openxr_to_blender_vec,
    openxr_to_blender_quat,
    vec_equal,
    quat_equal,
)
from common import setup_module, setup_function, teardown_function, teardown_module, STATE


# tests for viewer_pose are at the end of test_xr_session_settings.py (since they also test positional_tracking and absolute_tracking)


def test_navigation_location_change_is_applied():
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_pose_location = Vector((100, 200, 300))

    sim = STATE["sim"]
    headset = sim.device("/user/head")
    headset.position = blender_to_openxr_vec((10, 20, 30))

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state
    state.navigation_location = Vector((1000, 2000, 3000))

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = Vector(state.viewer_pose_location)
        loc -= state.navigation_location  # adjust for navigation pose
        loc -= settings.base_pose_location  # adjust for base pose

        # compare against the actual headset pose (after converting to Blender's frame-of-reference).
        # we effectively remove nav and base pose to return to the headset's pose
        headset_pos_bl = openxr_to_blender_vec(headset.position)
        assert vec_equal(loc, headset_pos_bl), f"{loc} != {headset_pos_bl}"

        # transform the navigation pose (in Blender's frame-of-reference)
        state.navigation_location += Vector((0.02, 0, 0))

        yield  # until the next frame


def test_navigation_rotation_change_is_applied():
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_pose_angle = radians(30)

    sim = STATE["sim"]
    headset = sim.device("/user/head")
    headset.orientation = blender_to_openxr_quat(Quaternion((0, 1, 0), radians(10)))

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state
    state.navigation_rotation = Quaternion((0, 1, 0), radians(20))

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        rot = Quaternion(state.viewer_pose_rotation)
        rot = state.navigation_rotation.inverted() @ rot  # adjust for navigation pose
        rot = Quaternion((0, 0, 1), -settings.base_pose_angle) @ rot  # adjust for base pose

        # compare against the actual headset pose (after converting to Blender's frame-of-reference)
        headset_rot_bl = openxr_to_blender_quat(headset.orientation)
        assert quat_equal(rot, headset_rot_bl), f"{rot.to_euler()} != {headset_rot_bl.to_euler()}"

        # transform the navigation pose (in Blender's frame-of-reference)
        state.navigation_rotation = Quaternion(state.navigation_rotation) @ Quaternion((1, 1, 1), 0.02)

        yield  # until the next frame


def test_navigation_scale_change_is_applied():
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_scale = 5.0

    sim = STATE["sim"]
    headset = sim.device("/user/head")
    headset.position = blender_to_openxr_vec((10, 20, 30))

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state
    state.navigation_scale = 10.0

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = Vector(state.viewer_pose_location)
        loc /= state.navigation_scale  # adjust for navigation pose
        loc /= settings.base_scale  # adjust for base scale

        # compare against the actual headset pose (after converting to Blender's frame-of-reference)
        headset_pos_bl = openxr_to_blender_vec(headset.position)
        assert vec_equal(loc, headset_pos_bl), f"{loc} != {headset_pos_bl}"

        # transform the navigation pose (in Blender's frame-of-reference)
        state.navigation_scale *= 2.0

        yield  # until the next frame


def test_navigation_pose_change_is_applied():
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_pose_location = Vector((100, 200, 300))
    settings.base_pose_angle = radians(30)
    settings.base_scale = 5.0

    sim = STATE["sim"]
    headset = sim.device("/user/head")
    headset.position = blender_to_openxr_vec((10, 20, 30))
    headset.orientation = blender_to_openxr_quat(Quaternion((0, 1, 0), radians(10)))

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state
    state.navigation_location = Vector((1000, 2000, 3000))
    state.navigation_rotation = Quaternion((0, 1, 0), radians(20))
    state.navigation_scale = 10.0

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = Vector(state.viewer_pose_location)
        rot = Quaternion(state.viewer_pose_rotation)

        # adjust for navigation pose
        loc -= state.navigation_location
        loc /= state.navigation_scale
        loc = state.navigation_rotation.inverted() @ loc
        rot = state.navigation_rotation.inverted() @ rot

        # adjust for base position and rotation
        loc -= settings.base_pose_location
        loc /= settings.base_scale
        loc = Quaternion((0, 0, 1), -settings.base_pose_angle) @ loc
        rot = Quaternion((0, 0, 1), -settings.base_pose_angle) @ rot

        # compare against the actual headset pose (after converting to Blender's frame-of-reference).
        # we effectively remove nav and base pose to return to the headset's pose
        headset_pos_bl = openxr_to_blender_vec(headset.position)
        headset_rot_bl = openxr_to_blender_quat(headset.orientation)
        assert vec_equal(loc, headset_pos_bl), f"{loc} != {headset_pos_bl}"
        assert quat_equal(rot, headset_rot_bl), f"{rot.to_euler()} != {headset_rot_bl.to_euler()}"

        # transform the navigation pose (in Blender's frame-of-reference)
        state.navigation_location += Vector((0.02, 0, 0))
        state.navigation_rotation = Quaternion(state.navigation_rotation) @ Quaternion((1, 1, 1), 0.02)
        state.navigation_scale *= 2.0

        yield  # until the next frame


if __name__ == "__main__":
    h = Harness(globals())
    h.run()
