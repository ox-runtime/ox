import bpy

from mathutils import Vector

from common import vec_equal, toggle_xr
from common import setup_module, setup_function, teardown_function, teardown_module, STATE


def test_start_xr():
    wm = bpy.context.window_manager

    assert wm.xr_session_state is None or not wm.xr_session_state.is_running(bpy.context)

    toggle_xr()

    assert wm.xr_session_state is not None
    assert wm.xr_session_state.is_running(bpy.context)


def test_restart_xr():
    wm = bpy.context.window_manager

    assert wm.xr_session_state is None

    for i in range(3):  # start, stop, start
        toggle_xr()

        if i % 2 == 0:  # starting xr
            assert wm.xr_session_state is not None
            assert wm.xr_session_state.is_running(bpy.context)
        else:
            assert wm.xr_session_state is None or not wm.xr_session_state.is_running(bpy.context)

        for i in range(10):  # wait for a few frames
            yield


def test_draw_handlers_work_in_xr():
    draw_frames = 0

    def draw_callback():
        nonlocal draw_frames
        draw_frames += 1

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    # start xr
    toggle_xr()

    for _ in range(10):  # wait for a few frames until the draw handler is called
        if draw_frames > 3:
            bpy.types.SpaceView3D.draw_handler_remove(handler, "XR")
            return  # test passed, draw handler was called in xr session

        yield

    bpy.types.SpaceView3D.draw_handler_remove(handler, "XR")
    assert False, "Draw handler was not called in XR session"


def test_has_valid_context_in_xr():
    context_valid = False

    def draw_callback():
        nonlocal context_valid

        assert bpy.context is not None, "bpy.context is None in draw handler"

        bpy.data.objects["Light"].location = Vector((1, 1, 1))
        new_loc = bpy.data.objects["Light"].location
        assert vec_equal(new_loc, Vector((1, 1, 1))), "Light location was not updated correctly in draw handler"

        context_valid = True

    handler = bpy.types.SpaceView3D.draw_handler_add(draw_callback, (), "XR", "POST_VIEW")

    # start xr
    toggle_xr()

    for _ in range(10):  # wait for a few frames until the draw handler is called
        if context_valid:
            bpy.types.SpaceView3D.draw_handler_remove(handler, "XR")
            return  # test passed, context was valid in draw handler

        yield

    bpy.types.SpaceView3D.draw_handler_remove(handler, "XR")
    assert False, "Draw handler was not called in XR session or context was invalid"


def test_app_timers_work_in_xr():
    timer_works = False

    def timer_callback():
        nonlocal timer_works

        view_layer = bpy.context.view_layer
        assert view_layer is not None, "bpy.context.view_layer is None in timer callback"

        timer_works = True

    bpy.app.timers.register(timer_callback)

    # start xr
    toggle_xr()

    for _ in range(10):  # wait for a few frames until the timer callback is called
        if timer_works:
            return  # test passed, timer callback was called in xr session

        yield

    assert False, "Timer callback was not called in XR session or context was invalid"
