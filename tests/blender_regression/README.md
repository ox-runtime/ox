# Regression tests for Blender's XR mode

Run using `/path/to/blender --python /path/to/harness.py`

This behaves like pytest, and will run all the test functions (starting with "test_") in files that start with "test_". E.g. "test_foo.py" containing "test_bar()".

You can also define the following functions in each test file:
- `setup_module()`: Called once before all tests in the module.
- `teardown_module()`: Called once after all tests in the module.
- `setup_function()`: Called before each test function.
- `teardown_function()`: Called after each test function.

This uses [ox_sim.py](https://github.com/ox-runtime/ox-sim-driver/blob/main/wrappers/python/ox_sim.py) to connect to the simulator device in [ox-runtime](https://github.com/ox-runtime/ox).
