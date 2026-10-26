# Companion projects

These projects support ox but are not part of the core runtime. Each one has its own README with full usage details.

## vog

[`vog`](https://github.com/ox-runtime/vog) is a small C++ window library built on GLFW and Dear ImGui. It handles window creation, DPI-aware font scaling, system dark-mode detection, native titlebar appearance, and other platform details. It includes Font Awesome 6 icons and a few ImGui widgets.

The simulator GUI uses it, but it is a general-purpose library. The API is a `Window` plus a frame callback. Put your ImGui calls in the callback. Do not call `NewFrame` or `Render` yourself.

Themes are `ThemeColors` and `ThemeVars` structs. Fields you leave unset fall back to the system dark or light palette. You can set a theme at startup through `WindowConfig::theme`, or change it later with `Window::SetTheme()`. The included widgets are a toggle switch, a combo box, a tooltip helper, and a colored button.

On Linux, vog uses the X11 backend by default. Wayland is opt-in with `-DVOG_WAYLAND=ON`, which needs the Wayland development packages. vog supports Windows, Linux, and macOS.

Use it when you want the simulator's window behavior without rebuilding the simulator. For the simulator itself, see [Simulator](simulator.md).

## pyopenxr_examples

[`pyopenxr_examples`](https://github.com/ox-runtime/pyopenxr_examples) contains small Python OpenXR programs built on `pyopenxr`. They also work as smoke tests for the runtime.

| Example | What it does |
|---|---|
| `hello_xr.py` | A Python version of Khronos' `hello_xr`. Multi-file. Draws cubes attached to reference spaces and reacts to triggers. |
| `color_cube.py` | One-file example with a large cube on the floor, using `xr.ContextObject`. |
| `debug_all_the_things.py` | Seven ways to add logging to a pyopenxr program. |
| `pink_world.py` | Minimal OpenGL rendering example. |
| `track_hmd2.py` | Prints headset poses. |
| `track_controller.py` | Prints controller poses. |
| `vive_tracker.py` | Prints Vive Tracker poses, skipping devices with a handheld-object role. |

For ox work, `track_hmd2.py` and `track_controller.py` are the quickest check that the runtime loaded, the driver resolved, and poses are moving. The rendering examples exercise swapchains and the graphics backends. `vive_tracker.py` is the check for tracker-only profiles.

The examples need `pyopenxr` installed, usually in a virtual environment. Run them with `XR_RUNTIME_JSON` set. In split-process mode, start the `ox` host first. For in-process testing, also set `OX_USE_SIMULATOR=1`. See [development workflow](development-workflow.md).

## ox-driver-example

[`ox-driver-example`](https://github.com/ox-runtime/ox-driver-example) is the starting template for a real hardware driver. It implements only the parts of `ox_driver.h` that a driver needs: a headset that always reports connected, a fixed standing pose at `(0, 1.6, 0)`, standard 1920x1080 per-eye display properties at 90 Hz, and a `submit_frame_pixels` callback that receives rendered pixels.

It is deliberately small. It shows the shape of `ox_driver_register`, how to fill the callback struct, and how to build a driver folder the runtime can load. It is a better starting point than the simulator, which includes GUI and REST code that a hardware driver does not need.

The driver needs only `ox_driver.h` for the API and `openxr.h` for the OpenXR types. Its CMake fetches those headers. Pass `-DOX_RUNTIME_REPO=/path/to/ox-runtime` to use a local runtime checkout instead. A typical build is:

```bash
cmake -B build
cmake --build build --config Release
```

The output is a driver folder that you copy into the runtime's `drivers/` folder. It is not a full runtime bundle, so it has no host or simulator.

The full contract and a step-by-step walkthrough are in [drivers](drivers.md). The project README has the build and install commands.

## How they fit together

`vog` is window code. `pyopenxr_examples` is a set of OpenXR applications you can point at ox. `ox-driver-example` is hardware-facing code that plugs into the runtime. None of them is required to use ox for automated testing, but each is a useful reference for its layer.

## Related pages

- [Architecture](architecture.md)
- [Drivers](drivers.md)
- [Simulator](simulator.md)
- [Development workflow](development-workflow.md)
