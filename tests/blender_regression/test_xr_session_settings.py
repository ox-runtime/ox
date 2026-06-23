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


def test_base_pose_type_camera_is_tracked():
    cam_data = bpy.data.cameras.new(name="TestCamera")
    cam = bpy.data.objects.new("TestCamera", cam_data)
    bpy.context.collection.objects.link(cam)
    bpy.context.scene.camera = cam  # set the new camera as the active camera
    cam.location = Vector((42, 42, 42))

    bpy.context.window_manager.xr_session_settings.base_pose_type = "SCENE_CAMERA"

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = state.viewer_pose_location
        assert vec_equal(loc, cam.location), f"{loc} != {cam.location}"

        cam.location += Vector((0.02, 0, 0))

        yield  # until the next frame


def test_base_pose_type_object_is_tracked():
    ob = bpy.data.objects.new("TestObject", None)
    bpy.context.collection.objects.link(ob)
    ob.location = Vector((42, 42, 42))

    bpy.context.window_manager.xr_session_settings.base_pose_type = "OBJECT"
    bpy.context.window_manager.xr_session_settings.base_pose_object = ob

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = state.viewer_pose_location
        assert vec_equal(loc, ob.location), f"{loc} != {ob.location}"

        ob.location += Vector((0.02, 0, 0))

        yield  # until the next frame


def test_base_pose_type_custom_is_tracked():
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_pose_type = "CUSTOM"
    settings.base_pose_location = Vector((42, 42, 42))
    settings.base_pose_angle = 0
    settings.base_scale = 1.0

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = state.viewer_pose_location
        assert vec_equal(loc, settings.base_pose_location), f"{loc} != {settings.base_pose_location}"

        angle = state.viewer_pose_rotation.to_euler().z
        assert abs(angle - settings.base_pose_angle) < 0.001, f"{angle} != {settings.base_pose_angle}"

        settings.base_pose_location += Vector((0.02, 0, 0))
        settings.base_pose_angle += 0.05

        yield  # until the next frame


def headset_tracking_test(positional_tracking: bool):
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_pose_type = "CUSTOM"
    settings.base_pose_location = Vector((100, 200, 300))
    settings.base_pose_angle = radians(30)
    settings.base_scale = 1.0
    settings.use_positional_tracking = positional_tracking
    settings.use_absolute_tracking = True

    sim = STATE["sim"]
    headset = sim.device("/user/head")
    headset.position = blender_to_openxr_vec((10, 20, 30))
    headset.orientation = blender_to_openxr_quat(Quaternion((0, 1, 0), radians(10)))

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = Vector(state.viewer_pose_location)
        rot = Quaternion(state.viewer_pose_rotation)

        # adjust for base position and rotation
        loc -= Vector((100, 200, 300))
        loc = Quaternion((0, 0, 1), radians(-30)) @ loc
        rot = Quaternion((0, 0, 1), radians(-30)) @ rot

        # compare against the actual headset pose (after converting to Blender's frame-of-reference)
        headset_pos_bl = openxr_to_blender_vec(headset.position) if positional_tracking else Vector()
        headset_rot_bl = openxr_to_blender_quat(headset.orientation)
        assert vec_equal(loc, headset_pos_bl), f"{loc} != {headset_pos_bl}"
        assert quat_equal(rot, headset_rot_bl), f"{rot.to_euler()} != {headset_rot_bl.to_euler()}"

        # transform the headset (in OpenXR's frame-of-reference)
        headset.position = tuple(p + 0.02 for p in headset.position)
        headset.orientation = Quaternion(headset.orientation) @ Quaternion((1, 1, 1), 0.02)

        yield  # until the next frame


def test_headset_location_and_rotation_is_tracked_if_positional_tracking_is_enabled():
    yield from headset_tracking_test(positional_tracking=True)


def test_only_headset_rotation_is_tracked_if_positional_tracking_is_disabled():
    yield from headset_tracking_test(positional_tracking=False)


def headset_absolute_tracking_test(absolute_tracking: bool):
    settings = bpy.context.window_manager.xr_session_settings
    settings.base_pose_type = "CUSTOM"
    settings.base_pose_location = Vector((100, 200, 300))
    settings.base_pose_angle = radians(30)
    settings.base_scale = 1.0
    settings.use_positional_tracking = True
    settings.use_absolute_tracking = absolute_tracking

    sim = STATE["sim"]
    headset = sim.device("/user/head")
    headset.position = blender_to_openxr_vec((10, 20, 30))
    headset.orientation = blender_to_openxr_quat(Quaternion((0, 1, 0), radians(10)))

    # start xr
    toggle_xr()

    state = bpy.context.window_manager.xr_session_state

    for i in range(10):  # run for a few frames
        if i < 3:  # skip the first few frames to allow viewer pose to stabilize
            yield

        loc = Vector(state.viewer_pose_location)
        rot = Quaternion(state.viewer_pose_rotation)

        # adjust for base position and rotation
        loc -= Vector((100, 200, 300))
        loc = Quaternion((0, 0, 1), radians(-30)) @ loc
        rot = Quaternion((0, 0, 1), radians(-30)) @ rot

        # compare against the actual headset pose (after converting to Blender's frame-of-reference)
        if absolute_tracking:
            headset_pos_bl = openxr_to_blender_vec(headset.position)
        else:  # relative to starting headset position
            headset_pos_bl = openxr_to_blender_vec(Vector(headset.position) - blender_to_openxr_vec((10, 20, 30)))

        headset_rot_bl = openxr_to_blender_quat(headset.orientation)  # rot is always absolute
        assert vec_equal(loc, headset_pos_bl), f"{loc} != {headset_pos_bl}"
        assert quat_equal(rot, headset_rot_bl), f"{rot.to_euler()} != {headset_rot_bl.to_euler()}"

        # transform the headset (in OpenXR's frame-of-reference)
        headset.position = tuple(p + 0.02 for p in headset.position)
        headset.orientation = Quaternion(headset.orientation) @ Quaternion((1, 1, 1), 0.02)

        yield  # until the next frame


def test_absolute_headset_pose_is_tracked_if_absolute_tracking_is_enabled():
    yield from headset_absolute_tracking_test(absolute_tracking=True)


def test_relative_headset_pose_changes_are_tracked_if_absolute_tracking_is_disabled():
    yield from headset_absolute_tracking_test(absolute_tracking=False)
