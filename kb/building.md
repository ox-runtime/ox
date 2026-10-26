# Building

The top-level [`ox`](https://github.com/ox-runtime/ox) repo builds the host, the runtime, the IPC proxy, and the simulator into one output folder. CMake FetchContent downloads the component repos, so you do not need separate clones.

## Requirements

- CMake 3.27 or newer.
- A C++17 compiler.
- Platform dependencies:
  - **Windows**: Visual Studio with the Desktop C++ workload. The build uses the static MSVC runtime (`/MT`) so the runtime works with applications built with different runtime settings.
  - **Linux**: OpenGL, X11, and packaging headers. The documented Ubuntu/Debian set is:

    ```bash
    sudo apt-get update
    sudo apt-get install -y libgl1-mesa-dev libxcb-dev libx11-dev \
        libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
        libpng-dev pkg-config
    ```

  - **macOS**: Xcode command line tools. CMake finds the Metal, QuartzCore, and CoreFoundation frameworks automatically.

The docs use `ox\build\win-x64` on Windows and `ox\build\linux-x64` on Linux. The release workflow uses `build/win-x64`, `build/linux-x64`, `build/macos-arm64`, and `build/macos-x64`.

## Configure and build

From the `ox` repo root:

```bash
cmake -B build
cmake --build build --config Release
```

To build only the host and what it needs:

```bash
cmake --build build --target ox --config Release
```

The build fetches these component versions unless you point it at local folders:

| Component | Default version |
|---|---|
| `ox-runtime` | `v0.4.0` |
| `ox-ipc-proxy` | `v0.4.0` |
| `ox-sim-driver` | `v0.7.0` |

It also fetches `spdlog` v1.17.0, `dylib` v3.0.1, `yaml-cpp` 0.9.0, and `whereami`.

## Using local checkouts

Set any of these CMake variables to a local repo root:

- `OX_RUNTIME_REPO`
- `OX_IPC_PROXY_REPO`
- `OX_SIM_DRIVER_REPO`

```bash
cmake -B build \
    -DOX_RUNTIME_REPO=/path/to/ox-runtime \
    -DOX_IPC_PROXY_REPO=/path/to/ox-ipc-proxy \
    -DOX_SIM_DRIVER_REPO=/path/to/ox-sim-driver
cmake --build build --target ox --config Release
```

The top-level CMake checks that the folder has a `CMakeLists.txt`, then uses `FetchContent_Declare(... SOURCE_DIR ...)`. The component repos also accept `-DOX_RUNTIME_REPO` when you build them on their own, because they fetch `ox_driver.h` from the runtime.

## Output files

The build writes into `build/bin`:

| Output | Purpose |
|---|---|
| `ox` / `ox.exe` | The host. Loads one driver, starts the IPC server, and serves the GUI and REST API through the simulator. |
| `ox_runtime` shared library (`ox_runtime.dll`, `libox_runtime.so`, `libox_runtime.dylib`) | The OpenXR runtime that the loader loads. |
| `ox_runtime.json` | The runtime manifest, generated from `ox-runtime/ox_runtime.json.in`. This is the file `XR_RUNTIME_JSON` points at. Some docs and older releases call it `ox_openxr.json`. Use the file that exists in your build. |
| `ox_ipc_client` shared library | Loaded by the runtime in split-process mode. |
| `ox_ipc_server` shared library | Loaded by the host and given the real driver. |
| `drivers/simulator/ox_driver.*` | The simulator driver, copied by a post-build step. |
| `config.yaml` | Copied next to the host. |
| `tests/runtime/runtime_tests` | Runtime unit tests. |
| `tests/ipc_proxy_tests` | IPC proxy tests. |

The post-build step deletes and replaces `drivers/simulator`, so an old simulator folder does not survive a rebuild.

## config.yaml

The host reads `config.yaml` from its own folder, not from the current working directory. Settings are grouped by driver folder name:

```yaml
drivers:
  simulator:
    gui:
      enabled: true
    rest_api:
      enabled: true
      port: 8765
```

The config loader walks the map and joins nested keys with dots. `gui.enabled` becomes the key passed to the driver's `set_config_bool` callback. `rest_api.port` becomes an integer passed to `set_config_int`. The driver decides which keys it understands. Unknown keys are ignored.

For the simulator, `gui.enabled` starts the preview window, and `rest_api.enabled` starts the HTTP server on the configured port. With both disabled, the host is headless but still serves the IPC driver.

## Building a repo on its own

Each component repo can also be built alone:

```bash
# ox-runtime
cmake -B build
cmake --build build --config Release

# ox-ipc-proxy
cmake -B build
cmake --build build --config Release

# ox-sim-driver
cmake -B build
cmake --build build --config Release
```

Those builds make the component libraries and tests, but not the full `ox` host bundle. Use the top-level build when you want a working runtime, host, and driver together.

## Related pages

- [Development workflow](development-workflow.md)
- [Release process](release-process.md)
- [Architecture](architecture.md)
- [Development guide](../docs/development.md)
