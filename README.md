# ox

**WORK-IN-PROGRESS** - This is still heavily under-development and is not (yet) fully compliant with the OpenXR spec.

**ox** is a simple, lightweight, cross-platform OpenXR runtime for Windows, Linux and Mac. It supports OpenGL, Vulkan and Metal.

The primary purpose of **ox** is automated testing of OpenXR applications. It comes with a [virtual OpenXR device](https://github.com/ox-runtime/ox-simulator) which can be controlled programmatically (e.g. press a button, move the headset etc). The effect of these actions can then be verified in the OpenXR application that you're testing.

<img height="400" alt="image" src="https://github.com/user-attachments/assets/e2b888d6-2295-4aa7-8aa0-be7f1b620c08" />

## Usage
Download ox from the [latest release](https://github.com/ox-runtime/ox/releases).

ox can be used in three ways:
- Directly inside your test code and application process - Useful for automated testing (including CI runners).
- [GUI](docs/gui.md) - Useful for development and live testing.
- [REST API](docs/rest_api.md) - Useful for agentic development.

Here's an example for automatically testing [Blender's XR mode](https://github.com/cmdr2/blender-xr-regression-tests).

## Code

The code is organized into multiple repositories:
- [ox](https://github.com/ox-runtime/ox) (this repo): bundles the core sub-repos into a single build.
- [ox-sim-driver](https://github.com/ox-runtime/ox-sim-driver): the simulator GUI and C API (plus wrappers) for controlling the virtual device programmatically
- [ox-runtime](https://github.com/ox-runtime/ox-runtime): the OpenXR runtime implementation
- [ox-ipc-proxy](https://github.com/ox-runtime/ox-ipc-proxy): a shared library that both the runtime and driver service depend on for IPC communication

## Build

```bash
cmake -B build
cmake --build build --config Release
```

The build will be produced in `./build/bin/`.

### Working with local checkouts
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

## Using the Runtime

Set the `XR_RUNTIME_JSON` environment variable to point to the runtime manifest:

* **Windows:** `set XR_RUNTIME_JSON=C:\path\to\ox\build\bin\ox_openxr.json`
* **Linux:** `export XR_RUNTIME_JSON=/path/to/ox/build/bin/ox_openxr.json`

Then run any OpenXR application. Please open a [new issue](/issues/new) if you encounter any issues.

## Documentation

- [Simulator GUI reference](docs/gui.md)
- [Simulator REST API reference](docs/rest_api.md)
- [Simulator C API reference](docs/c_api.md)

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
- [OpenXR Loader Design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md)
