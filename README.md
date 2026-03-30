# ox

**WORK-IN-PROGRESS** - This is still a prototype and is not (yet) fully compliant with the OpenXR spec.

**ox** is a simple and lightweight OpenXR runtime for Windows, Linux and Mac. It supports OpenGL and Vulkan.

The primary purpose of **ox** is automated testing of OpenXR applications. It comes with a [virtual OpenXR device](https://github.com/ox-runtime/ox-simulator) which can be controlled programmatically (e.g. press a button, move the headset etc). The effect of these actions can then be verified in the OpenXR application that you're testing.

## Code

The code is organized into multiple repositories:
- [ox](https://github.com/ox-runtime/ox) (this repo): the host executable and CMake build orchestration
- [ox-sim-driver](https://github.com/ox-runtime/ox-sim-driver): the simulator GUI and C API for controlling the virtual device programmatically
- [ox-runtime](https://github.com/ox-runtime/ox-runtime): the OpenXR runtime implementation
- [ox-ipc-proxy](https://github.com/ox-runtime/ox-ipc-proxy): a shared library that both the runtime and simulator driver depend on for IPC communication

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

**Windows:**
```batch
set XR_RUNTIME_JSON=C:\path\to\ox\build\bin\ox_openxr.json
```

**Linux:**
```bash
export XR_RUNTIME_JSON=/path/to/ox/build/bin/ox_openxr.json
```

Then run any OpenXR application.

## Documentation

- [Simulator GUI reference](docs/gui.md)
- [Simulator REST API reference](docs/rest_api.md)
- [Simulator C API reference](docs/c_api.md)

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
- [OpenXR Loader Design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md)
