import os
import bpy
import sys
import inspect
import traceback
from pathlib import Path

script_dir = Path(__file__).parent.resolve()
sys.path.append(str(script_dir))

# Force Python to print the actual traceback whenever any error occurs
sys.excepthook = lambda exctype, value, tb: traceback.print_exception(exctype, value, tb)


class Harness:
    """
    Looks for files starting with "test_", containing functions starting with "test_".

    Runs tests across multiple Blender frames to ensure that the XR state is applied properly.

    This is achieved by running each test function as a generator (which yields after each frame).

    Each test function's generator is called once per timer callback, until the generator completes or raises an exception.
    This way, each test can yield and continue in the next timer callback, allowing it to run across multiple frames.
    """

    FRAME_INTERVAL = 1.0 / 30.0  # Run at 30 FPS

    def __init__(self):
        self.test_modules = []
        self.test_results = []
        self.curr_test_module = None
        self.curr_test_fn = None
        self.curr_test_gen = None
        self.state = "GET_NEXT_MODULE"

    def _discover_tests(self):
        # list all files starting with "test_" in the current directory
        dir_path = os.path.dirname(os.path.abspath(__file__))
        test_files = [f for f in os.listdir(dir_path) if f.startswith("test_")]

        # load each module
        for test_file in test_files:
            self.test_modules.append(TestFileHarness(self, test_file))

        num_tests = sum(len(m.tests) for m in self.test_modules)
        self._log(f"Discovered {num_tests} tests")

        # convert to iterators
        for mod in self.test_modules:
            mod.tests = iter(mod.tests)
        self.test_modules = iter(self.test_modules)

    def _on_frame(self):
        # state machine:
        # GET_NEXT_MODULE ->
        #   START_MODULE -> [GET_NEXT_TEST -> START_TEST -> RUN_GENERATOR -> STOP_TEST -> GET_NEXT_TEST] -> END_MODULE
        #   -> GET_NEXT_MODULE
        # -> END_TESTING

        # print(self.state, self.curr_test_module, self.curr_test_fn, self.curr_test_gen)
        try:
            if self.state == "GET_NEXT_MODULE":
                self.curr_test_module = next(self.test_modules, None)
                self.state = "START_MODULE" if self.curr_test_module else "END_TESTING"

            elif self.state == "START_MODULE":
                self._log("Starting tests..")
                self.curr_test_module.fixtures["setup_module"]()
                self.state = "GET_NEXT_TEST"

            elif self.state == "GET_NEXT_TEST":
                self.curr_test_fn = next(self.curr_test_module.tests, None)

                if self.curr_test_fn:
                    self.curr_test_module.fixtures["setup_function"]()
                    self.state = "START_TEST"
                else:
                    self.state = "END_MODULE"

            elif self.state == "START_TEST":
                test_fn_name = self.curr_test_module.filename + "::" + self.curr_test_fn.__name__
                self._log(f"Running test: {test_fn_name}")
                try:
                    self.curr_test_gen = self.curr_test_fn()  # start the generator fn

                    if self.curr_test_gen and inspect.isgenerator(self.curr_test_gen):
                        self.state = "RUN_GENERATOR"
                    else:  # test passed without yielding
                        self._record_test_success(test_fn_name)
                        self.state = "STOP_TEST"
                except Exception as e:
                    self._record_test_failure(test_fn_name, e)
                    self.state = "STOP_TEST"

            elif self.state == "RUN_GENERATOR":
                test_fn_name = self.curr_test_module.filename + "::" + self.curr_test_fn.__name__
                try:
                    next(self.curr_test_gen)  # run the test fn until the next yield
                except StopIteration:
                    self._record_test_success(test_fn_name)
                    self.state = "STOP_TEST"
                except Exception as e:
                    self._record_test_failure(test_fn_name, e)
                    self.state = "STOP_TEST"

            elif self.state == "STOP_TEST":
                self.curr_test_module.fixtures["teardown_function"]()
                self.state = "GET_NEXT_TEST"

            elif self.state == "END_MODULE":
                self.curr_test_module.fixtures["teardown_module"]()
                self.state = "GET_NEXT_MODULE"

            elif self.state == "END_TESTING":
                self._print_results()
                self._log("Test complete! Check the console for results.")
                bpy.ops.wm.quit_blender()
                return
        except Exception as e:
            print("Harness encountered an exception:")
            traceback.print_exc()
            bpy.ops.wm.quit_blender()
            return

        return Harness.FRAME_INTERVAL

    def _print_results(self):
        print("=" * 40)
        print("Test results:")
        print("=" * 40)

        for result in self.test_results:
            print(f"{'OK' if result[1] else 'FAIL'} \t {result[0]} \t {result[2]}")

        print("=" * 40)

    def _record_test_success(self, test_fn_name):
        self.test_results.append((test_fn_name, True, ""))

    def _record_test_failure(self, test_fn_name, exception):
        self._log(f"Test {test_fn_name} failed")
        traceback.print_exc()
        self.test_results.append((test_fn_name, False, str(exception)))

    def _log(self, message):
        print(message)
        bpy.context.workspace.status_text_set(message)

    def run(self):
        self._discover_tests()
        bpy.app.timers.register(self._on_frame)


class TestFileHarness:
    def __init__(self, harness, filename):
        self.harness = harness
        self.filename = filename
        self.fixtures = {
            "setup_module": None,
            "setup_function": None,
            "teardown_module": None,
            "teardown_function": None,
        }

        # load the module
        self.module = __import__(self.filename[:-3])  # remove ".py" extension

        # discover tests
        self.tests = [t for t in self.module.__dict__.values() if callable(t) and t.__name__.startswith("test_")]

        for fixture in self.fixtures.keys():
            self.fixtures[fixture] = getattr(self.module, fixture, lambda: None)


if __name__ == "__main__":
    h = Harness()
    h.run()
