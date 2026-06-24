# ox

**ox** is a new lightweight and cross-platform OpenXR runtime for Windows, Linux and Mac. It supports OpenGL, Vulkan and Metal.

The primary purpose of **ox** is automated testing of OpenXR applications using a virtual OpenXR device. It is useful during development and CI testing.

You can control the virtual device programmatically (e.g. press a button, move the headset, read screen texture etc), and integrate it with your existing test framework and code.

> [!WARNING]
> **WORK-IN-PROGRESS** - **ox** is still heavily under-development and is not (yet) fully compliant with the OpenXR spec.

## Get Started
Click to use **ox** for:
* [Automated Testing](docs/automated_testing.md) - Use **ox** inside your application and testing framework. Useful for automated testing (including CI runners).
* [GUI](docs/gui.md) - See the headset's view and control the devices visually in a GUI window. Useful for development and live testing.
* [REST API](docs/rest_api.md) - Control the virtual device over HTTP. Useful for agentic development, or when you can't use the in-process API.

## Using the Runtime

Set the `XR_RUNTIME_JSON` environment variable, or use the [GUI](docs/gui.md) to set **ox** as the default OpenXR runtime.

* **Windows:** `set XR_RUNTIME_JSON=C:\path\to\ox\ox_openxr.json`
* **Linux/macOS:** `export XR_RUNTIME_JSON=/path/to/ox/ox_openxr.json`

Then run any OpenXR application. Please open a [new issue](https://github.com/ox-runtime/ox/issues/new) if you encounter any issues.

## Development
### Code

The code is organized into the following repositories:
- [ox](https://github.com/ox-runtime/ox) (this repo): bundles the core sub-repos into a single build.
- [ox-sim-driver](https://github.com/ox-runtime/ox-sim-driver): the simulator GUI and C API (plus wrappers) for controlling the virtual device programmatically.
- [ox-runtime](https://github.com/ox-runtime/ox-runtime): the OpenXR runtime implementation.
- [ox-ipc-proxy](https://github.com/ox-runtime/ox-ipc-proxy): decouples the application process from the XR device, with an IPC proxy. Used by the GUI and REST API.

### Build
1. (Linux-only) Install platform dependencies:

```bash
# Ubuntu / Debian
sudo apt-get update
sudo apt-get install -y libgl1-mesa-dev libxcb-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libpng-dev pkg-config
```
2. Configure and build:

```bash
cmake -B build
cmake --build build --config Release
```

The build will be produced in `./build/bin/`.

#### Working with local checkouts
By default, the top-level CMake build fetches the sub-project repos with `FetchContent`. To work against local repo clones instead, pass any of these build-time flags:

- `OX_RUNTIME_REPO`
- `OX_IPC_PROXY_REPO`
- `OX_SIM_DRIVER_REPO`

For e.g. to build against local clones of all three repos:

```bash
cmake -B build \
	-DOX_RUNTIME_REPO=/path/to/ox-runtime \
	-DOX_IPC_PROXY_REPO=/path/to/ox-ipc-proxy \
	-DOX_SIM_DRIVER_REPO=/path/to/ox-sim-driver
cmake --build build --target ox --config Release
```

## Documentation

- [Automated Testing](docs/automated_testing.md)
- [GUI](docs/gui.md)
- [REST API](docs/rest_api.md)
- [C API](docs/c_api.md)

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
- [OpenXR Loader Design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md)
