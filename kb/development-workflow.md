# Development workflow

This page covers the usual loop: build, run the host, point an application at ox, and test.

## Build once

Follow [Building](building.md). For quick work on one component, use a local checkout flag so CMake does not fetch a tagged copy:

```bash
cmake -B build \
    -DOX_RUNTIME_REPO=../ox-runtime \
    -DOX_IPC_PROXY_REPO=../ox-ipc-proxy \
    -DOX_SIM_DRIVER_REPO=../ox-sim-driver
cmake --build build --target ox --config Release
```

Rebuild only what changed. Useful targets include `ox`, `ox_runtime`, `ox_ipc_client`, `ox_ipc_server`, `ox-simulator`, `runtime_tests`, and `ipc_proxy_tests`.

## Run the host

Start the host from the folder that contains `config.yaml` and the `drivers` folder:

```bash
cd build/bin
./ox          # Linux/macOS
ox.exe        # Windows
```

You should see the simulator GUI and the host log. The REST API starts on port `8765` if the config enables it.

The host expects exactly one driver folder under `drivers/`. A normal build gives you `drivers/simulator`. If you add a custom driver, move the simulator folder out of the way while you test it.

## Point an application at ox

In the terminal that will run the application:

```bash
export XR_RUNTIME_JSON=/path/to/ox/build/bin/ox_runtime.json
```

Windows:

```powershell
$env:XR_RUNTIME_JSON = "C:\path\to\ox\build\bin\ox_runtime.json"
```

For in-process testing, also set:

```bash
export OX_USE_SIMULATOR=1
```

For GUI and REST work, leave `OX_USE_SIMULATOR` unset. The runtime then loads `ox_ipc_client` and connects to the host.

## Smoke tests

The `pyopenxr_examples` repo has small programs that print device state:

```bash
python track_hmd2.py
python track_controller.py
```

Run them with `XR_RUNTIME_JSON` set. With the host running, they connect through IPC and print poses. With `OX_USE_SIMULATOR=1`, they run fully in-process. These are the fastest way to check that the runtime loaded, the driver was found, and poses are moving.

`hello_xr.py`, `color_cube.py`, and `pink_world.py` exercise rendering and swapchains. Use them when you change graphics code.

## Unit tests

Each repo has its own tests:

```bash
# From ox-runtime or the top-level build
cmake --build build --target runtime_tests --config Release
./build/bin/tests/runtime/runtime_tests

# From ox-ipc-proxy or the top-level build
cmake --build build --target ipc_proxy_tests --config Release
./build/bin/tests/ipc_proxy_tests

# From ox-sim-driver
cmake --build build --target ox_sim_tests --config Release
./build/bin/tests/ox_sim_tests
```

The runtime tests use an injected mock driver. They do not need the host, the simulator, or a GPU. The IPC tests use a fake driver on both sides. The simulator tests cover profiles, devices, inputs, frames, and the slider math.

Run the relevant suite before you send a change. If you touch the driver API or the frame path, run more than one.

## Blender end-to-end

The Blender test suite is the broadest real-world check:

```bash
export XR_RUNTIME_JSON=/path/to/ox/build/bin/ox_runtime.json
export OX_USE_SIMULATOR=1
blender --python /path/to/blender-xr-regression-tests/harness.py
```

Use the filter arguments after `--` to keep a run short while you work. See [Testing an application](testing.md).

## Common problems

### The application crashes on startup

- Check that `XR_RUNTIME_JSON` points at a file that exists. The build creates `ox_runtime.json`. Older docs say `ox_openxr.json`. Pointing at a missing file or a stale copy is the most common mistake.
- Check that the runtime and driver use the same configuration. A Debug runtime with a Release driver, or the reverse, can fail to load or crash.
- In split-process mode, check that the host is running and that the IPC client can see the shared memory. A host crash can leave stale state behind. Restart the host.

### The host exits right away

- Check that `drivers/` contains exactly one folder.
- Check that the driver library has the expected name: `ox_driver.dll`, `ox_driver.so`, or `ox_driver.dylib`.
- Check the log for a missing required callback or a failed `initialize()`.

### The GUI does not appear

- Check `config.yaml` next to the executable. `gui.enabled` must be true.
- On macOS, the GUI must run on the main thread. The host handles the ordering, but a custom embedder may not.
- On Linux, install the X11 and GLFW packages listed in [Building](building.md).

### Tests pass in-process but fail through the host

The split path adds a control channel, shared memory, and a 90 Hz frame-forwarding loop. Check that the host is alive, that only one client is connected, and that you are not reading a frame before the server has forwarded one. Run with `OX_USE_SIMULATOR=1` to separate the application from IPC.

## CI

The release workflow builds and runs `runtime_tests` and `ipc_proxy_tests` on every target platform before packaging. A failing unit test on one platform blocks that platform's release assets. It is not just a warning.

See [release process](release-process.md) for the workflow, and [`release.yml`](https://github.com/ox-runtime/ox/blob/main/.github/workflows/release.yml) for the exact commands.

## Related pages

- [Building](building.md)
- [Release process](release-process.md)
- [Testing an application](testing.md)
- [Development guide](../docs/development.md)
