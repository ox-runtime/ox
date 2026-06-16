import bpy
import sys
import inspect
import traceback

# Force Python to print the actual traceback whenever any error occurs
sys.excepthook = lambda exctype, value, tb: traceback.print_exception(exctype, value, tb)


class Harness:
    """
    Runs tests across multiple Blender frames to ensure XR state is properly applied.

    This is achieved by running each test function as a generator (which yields after each frame).

    Each test function's generator is called once per timer callback, until the generator completes or raises an exception.
    This way, each test can yield and continue in the next timer callback, allowing it to run across multiple frames.
    """

    FRAME_INTERVAL = 1.0 / 30.0  # Run at 30 FPS

    def __init__(self, globals_dict):
        self.globals = globals_dict
        self.tests = None
        self.fixtures = {
            "setup_module": None,
            "setup_function": None,
            "teardown_module": None,
            "teardown_function": None,
        }

        self.test_results = []
        self.curr_test_fn = None
        self.curr_test_gen = None
        self.state = "START_MODULE"

    def _discover_tests(self):
        self.tests = [t for t in self.globals.values() if callable(t) and t.__name__.startswith("test_")]
        print(f"Discovered {len(self.tests)} tests")
        self.tests = iter(self.tests)

        for fixture in self.fixtures.keys():
            self.fixtures[fixture] = self.globals.get(fixture, lambda: None)

    def _print_results(self):
        print("=" * 40)
        print("Test results:")
        print("=" * 40)

        for result in self.test_results:
            print(f"{'OK' if result[1] else 'FAIL'} \t {result[0]} \t {result[2]}")

        print("=" * 40)

    def _record_test_success(self, test_fn):
        self.test_results.append((test_fn.__name__, True, ""))

    def _record_test_failure(self, test_fn, exception):
        print(f"Test {test_fn.__name__} failed with exception:")
        traceback.print_exc()
        self.test_results.append((test_fn.__name__, False, str(exception)))

    def _on_frame(self):
        # state machine:
        # START_MODULE -> [GET_NEXT_TEST -> START_TEST -> RUN_GENERATOR -> STOP_TEST -> GET_NEXT_TEST] -> END_MODULE
        try:
            if self.state == "START_MODULE":
                self._show_toast("Starting tests..")
                self.fixtures["setup_module"]()
                self.state = "GET_NEXT_TEST"

            elif self.state == "GET_NEXT_TEST":
                self.curr_test_fn = next(self.tests, None)
                self.state = "START_TEST" if self.curr_test_fn else "END_MODULE"

            elif self.state == "START_TEST":
                self.fixtures["setup_function"]()
                print(f"Starting test: {self.curr_test_fn.__name__}")
                self._show_toast(f"Starting test: {self.curr_test_fn.__name__}")
                try:
                    self.curr_test_gen = self.curr_test_fn()  # start the generator fn

                    if self.curr_test_gen and inspect.isgenerator(self.curr_test_gen):
                        self.state = "RUN_GENERATOR"
                    else:  # test passed without yielding
                        self._record_test_success(self.curr_test_fn)
                        self.state = "STOP_TEST"
                except Exception as e:
                    self._record_test_failure(self.curr_test_fn, e)
                    self.state = "STOP_TEST"

            elif self.state == "RUN_GENERATOR":
                try:
                    next(self.curr_test_gen)  # run the test fn until the next yield
                except StopIteration:
                    self._record_test_success(self.curr_test_fn)
                    self.state = "STOP_TEST"
                except Exception as e:
                    self._record_test_failure(self.curr_test_fn, e)
                    self.state = "STOP_TEST"

            elif self.state == "STOP_TEST":
                self.fixtures["teardown_function"]()
                self.state = "GET_NEXT_TEST"

            elif self.state == "END_MODULE":
                self._print_results()
                self._show_toast("Test complete! Check the console for results.")
                self.fixtures["teardown_module"]()
                bpy.ops.wm.quit_blender()
                return
        except Exception as e:
            print("Harness encountered an exception:")
            traceback.print_exc()
            bpy.ops.wm.quit_blender()
            return

        return Harness.FRAME_INTERVAL

    def _show_toast(self, message):
        def draw(self, context):
            self.layout.label(text=message)

        bpy.context.window_manager.popup_menu(draw, title="Test")

    def run(self):
        self._discover_tests()
        bpy.app.timers.register(self._on_frame)
