import bpy

from dataclasses import dataclass
from typing import Union

from common import toggle_xr
from common import (
    setup_module as setup_module_orig,
    setup_function as setup_function_orig,
    teardown_function,
    teardown_module,
    STATE,
)

import actions_helper
import actions_helper.actionset


calls = []


def setup_module():
    setup_module_orig()

    def event_callback(event_type, event):
        xr = event.xr
        calls.append((xr.action, event.value, xr.state[0] if xr.state else None))

    actions_helper.event_callback = event_callback


def setup_function():
    setup_function_orig()

    calls.clear()


@dataclass
class ButtonTest:
    action: str  # action name in actions_helper.actionset.actions
    path: str  # openxr component path

    # value to use in set_input calls
    on_val: Union[float, int, bool]
    off_val: Union[float, int, bool]

    # expected event.state value (uses on_val/off_val if None)
    on_exp: Union[float, None] = None
    off_exp: Union[float, None] = None


def test_controller_pose_actions_set():
    # start XR
    toggle_xr()

    # bind the action set for Quest buttons, and set the action set as active
    actions_helper.bind_actions()

    wm = bpy.context.window_manager
    state = wm.xr_session_state

    assert len(state.actionmaps) > 0
    assert len(state.actionmaps[-1].actionmap_items) == len(actions_helper.actionset.actions)


def test_controller_trigger_and_squeeze_solo():
    toggle_xr()
    actions_helper.bind_actions()

    sim = STATE["sim"]

    button_states = {"on_val": 1.0, "off_val": 0.0}
    tests = [
        ButtonTest("trigger", "/input/trigger/value", **button_states),
        ButtonTest("squeeze", "/input/squeeze/value", **button_states),
    ]

    for hand in ("left", "right"):
        controller = sim.device(f"/user/hand/{hand}")

        yield from run_tests(controller, tests)


def test_controller_trigger_and_squeeze_simultaneous():
    toggle_xr()
    actions_helper.bind_actions()

    sim = STATE["sim"]

    button_states = {"on_val": 1.0, "off_val": 0.0}
    tests = [
        [  # both pressed in the same frame
            ButtonTest("trigger", "/input/trigger/value", **button_states),
            ButtonTest("squeeze", "/input/squeeze/value", **button_states),
        ],
    ]

    for hand in ("left", "right"):
        controller = sim.device(f"/user/hand/{hand}")

        yield from run_tests(controller, tests)


def test_push_buttons_solo():
    toggle_xr()
    actions_helper.bind_actions()

    sim = STATE["sim"]

    button_states = {"on_val": True, "off_val": False, "on_exp": 1.0, "off_exp": 0.0}
    tests = {
        "left": [
            ButtonTest("button_a_lefthand", "/input/x/click", **button_states),
            ButtonTest("button_b_lefthand", "/input/y/click", **button_states),
            ButtonTest("button_a_touch_lefthand", "/input/x/touch", **button_states),
            ButtonTest("button_b_touch_lefthand", "/input/y/touch", **button_states),
        ],
        "right": [
            ButtonTest("button_a_righthand", "/input/a/click", **button_states),
            ButtonTest("button_b_righthand", "/input/b/click", **button_states),
            ButtonTest("button_a_touch_righthand", "/input/a/touch", **button_states),
            ButtonTest("button_b_touch_righthand", "/input/b/touch", **button_states),
        ],
    }

    for hand in tests:
        controller = sim.device(f"/user/hand/{hand}")
        hand_tests = tests[hand]

        yield from run_tests(controller, hand_tests)


def test_push_button_touch_and_press():
    toggle_xr()
    actions_helper.bind_actions()

    sim = STATE["sim"]

    button_states = {"on_val": True, "off_val": False, "on_exp": 1.0, "off_exp": 0.0}
    tests = {
        "left": [
            [  # we're touching and pressing, so assert that both receive callbacks
                ButtonTest("button_a_lefthand", "/input/x/click", **button_states),
                ButtonTest("button_a_touch_lefthand", "/input/x/touch", **button_states),
            ],
            [
                ButtonTest("button_b_lefthand", "/input/y/click", **button_states),
                ButtonTest("button_b_touch_lefthand", "/input/y/touch", **button_states),
            ],
        ],
        "right": [
            [
                ButtonTest("button_a_righthand", "/input/a/click", **button_states),
                ButtonTest("button_a_touch_righthand", "/input/a/touch", **button_states),
            ],
            [
                ButtonTest("button_b_righthand", "/input/b/click", **button_states),
                ButtonTest("button_b_touch_righthand", "/input/b/touch", **button_states),
            ],
        ],
    }

    for hand in tests:
        controller = sim.device(f"/user/hand/{hand}")
        hand_tests = tests[hand]

        yield from run_tests(controller, hand_tests)


def test_joysticks():
    toggle_xr()
    actions_helper.bind_actions()

    sim = STATE["sim"]

    for dir in (1, -1):
        button_states = {"on_val": dir * 0.8, "off_val": 0.0}
        joystick_tests = {
            "left": [
                ButtonTest("joystick_x_lefthand", "/input/thumbstick/x", **button_states),
                ButtonTest("joystick_y_lefthand", "/input/thumbstick/y", **button_states),
            ],
            "right": [
                ButtonTest("joystick_x_righthand", "/input/thumbstick/x", **button_states),
                ButtonTest("joystick_y_righthand", "/input/thumbstick/y", **button_states),
            ],
        }

        for hand in joystick_tests:
            controller = sim.device(f"/user/hand/{hand}")
            hand_tests = joystick_tests[hand]

            yield from run_tests(controller, hand_tests)


def test_push_button_and_trigger_simultaneous():
    toggle_xr()
    actions_helper.bind_actions()

    sim = STATE["sim"]

    push_button_states = {"on_val": True, "off_val": False, "on_exp": 1.0, "off_exp": 0.0}
    axis_button_states = {"on_val": 1.0, "off_val": 0.0}
    tests = [
        [
            ButtonTest("button_a_lefthand", "/input/x/click", **push_button_states),
            ButtonTest("squeeze", "/input/squeeze/value", **axis_button_states),
        ],
    ]

    controller = sim.device(f"/user/hand/left")

    yield from run_tests(controller, tests)


def run_tests(device, tests):
    for test in tests:
        test = test if isinstance(test, list) else [test]
        for button in test:
            device.set_input(button.path, button.off_val)

        yield

        for button in test:
            device.set_input(button.path, button.on_val)

        yield  # yield for 2 frames
        yield

        for button in test:
            device.set_input(button.path, button.off_val)

        yield

        for button in test:
            action = button.action
            on_expected = button.on_exp if button.on_exp is not None else button.on_val
            off_expected = button.off_exp if button.off_exp is not None else button.off_val

            assert has_call((action, "PRESS", on_expected)), f"{action} wasn't pressed: {calls}"
            assert has_call((action, "RELEASE", off_expected)), f"{action} wasn't released: {calls}"
            assert get_call_index((action, "PRESS", on_expected)) < get_call_index((action, "RELEASE", off_expected))

        calls.clear()


def has_call(expected_call):
    return get_call_index(expected_call) != -1


def get_call_index(expected_call):
    for i, call in enumerate(calls):
        btn, value, state = call
        exp_btn, exp_value, exp_state = expected_call
        if btn == exp_btn and value == exp_value and abs(state - exp_state) < 0.001:
            return i
    return -1
