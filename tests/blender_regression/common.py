import bpy
from math import radians
from mathutils import Quaternion, Vector

from ox_sim import Simulator

EPSILON = 0.0001

# This basis handles the OpenXR-to-Blender 3-way axis swap
OXR_TO_BLENDER_BASIS = Quaternion((-0.5, 0.5, 0.5, 0.5))
BLENDER_TO_OXR_BASIS = OXR_TO_BLENDER_BASIS.inverted()

# The baseline look-at offset matrix (-90 degrees pitch around X)
BASELINE_OFFSET = Quaternion((1, 0, 0), radians(-90))
BASELINE_OFFSET_INV = BASELINE_OFFSET.inverted()


STATE = {
    "sim": None,
}


def setup_module():
    STATE["sim"] = Simulator()
    STATE["sim"].profile = "oculus_quest_3"


def setup_function():
    wm = bpy.context.window_manager

    settings = wm.xr_session_settings
    settings.base_pose_type = "CUSTOM"
    settings.base_pose_location = Vector()
    settings.base_pose_angle = 0
    settings.base_scale = 1.0
    settings.use_positional_tracking = True
    settings.use_absolute_tracking = True

    # reset devices
    sim = STATE["sim"]
    for device in ("/user/head", "/user/hand/right", "/user/hand/left"):
        device = sim.device(device)
        device.position = blender_to_openxr_vec((0, 0, 0))
        device.orientation = blender_to_openxr_quat(Quaternion())

    # stop a running xr session before starting the next test
    if wm.xr_session_state is not None and wm.xr_session_state.is_running(bpy.context):
        print("Stopping running XR session before starting next test")
        toggle_xr()
        print("Stopped")


def teardown_function():
    pass


def teardown_module():
    STATE["sim"].shutdown()
    STATE["sim"] = None


def toggle_xr():
    area = next((a for a in bpy.context.screen.areas if a.type == "VIEW_3D"), None)

    with bpy.context.temp_override(area=area):
        bpy.ops.wm.xr_session_toggle()


def openxr_to_blender_vec(p):
    # Blender: Right is +X, Forward is +Y, Up is +Z
    # OpenXR: Right is +X, Forward is -Z, Up is +Y
    p = Vector(p) if isinstance(p, tuple) else p
    return Vector((p.x, -p.z, p.y))


def blender_to_openxr_vec(p):
    p = Vector(p) if isinstance(p, tuple) else p
    return Vector((p.x, p.z, -p.y))


def openxr_to_blender_quat(q):
    # Empirical evidence, probably due to how the viewer camera faces down in Blender:
    # Rotating around +X axis in OpenXR rotates the -Y axis in Blender
    # Rotating around +Y axis in OpenXR rotates the -Z axis in Blender
    # Rotating around +Z axis in OpenXR rotates the -X axis in Blender

    q = Quaternion(q)
    structural_rot = BLENDER_TO_OXR_BASIS @ q @ OXR_TO_BLENDER_BASIS
    structural_rot.conjugate()
    return structural_rot @ BASELINE_OFFSET


def blender_to_openxr_quat(q):
    q = Quaternion(q)
    stripped = q @ BASELINE_OFFSET_INV
    stripped.conjugate()
    return OXR_TO_BLENDER_BASIS @ stripped @ BLENDER_TO_OXR_BASIS


def vec_equal(a: Vector, b: Vector) -> float:
    a = Vector(a)
    b = Vector(b)
    return (a - b).length <= EPSILON


def quat_equal(a: Quaternion, b: Quaternion) -> float:
    a = Quaternion(a)
    b = Quaternion(b)
    return a.rotation_difference(b).angle <= EPSILON
