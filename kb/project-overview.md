# Project overview

ox is an OpenXR runtime built for automated testing. It gives an OpenXR application a virtual headset and controllers. A test can move the headset, press a button, read the rendered eye image, and check the result.

ox is not meant to replace SteamVR or Monado for everyday VR. It is a testing tool that behaves like a real runtime so an application cannot tell the difference.

## Status

ox is a work in progress. It is not yet fully compliant with the OpenXR spec. The top priority is full spec compliance, including passing Khronos' OpenXR Conformance Test Suite. A testing tool that disagrees with the spec gives wrong test results.

The parts that exist are already used for real regression testing, including Blender's XR mode. Treat it as usable but still changing.

## What it does

- Runs an OpenXR application against a simulated headset and controllers.
- Simulates Meta Quest 2 and 3, HTC Vive, Valve Index, and Vive Trackers.
- Lets you control the devices from a GUI, an HTTP API, a C API, or a Python wrapper.
- Captures the frames the application sends, so a test can look at pixels.
- Runs on Windows, Linux, and macOS, with OpenGL, Vulkan, and Metal.

The simulator is implemented as a driver, the same as real hardware would be. See [simulator](simulator.md) and [drivers](drivers.md).

## Supported platforms

| Platform | Graphics APIs |
|---|---|
| Windows | OpenGL, Vulkan |
| Linux | OpenGL, Vulkan |
| macOS | OpenGL, Vulkan, Metal |

macOS support includes older Intel machines. The project aims to run on a wide range of hardware, not just current headsets.

## What each repo does

| Repository | Role |
|---|---|
| [`ox`](https://github.com/ox-runtime/ox) | The top-level build. Fetches the other repos, builds the `ox` host, the runtime library, and the manifest. |
| [`ox-runtime`](https://github.com/ox-runtime/ox-runtime) | The OpenXR runtime. Loads a driver and asks it for poses, inputs, and frames. |
| [`ox-ipc-proxy`](https://github.com/ox-runtime/ox-ipc-proxy) | Moves driver calls between two processes. `ox_ipc_client` runs in the application. `ox_ipc_server` runs in the host. |
| [`ox-sim-driver`](https://github.com/ox-runtime/ox-sim-driver) | The virtual device driver, plus the simulator GUI, REST API, C API, and language wrappers. |
| [`ox-driver-example`](https://github.com/ox-runtime/ox-driver-example) | A small template driver for real hardware. |
| [`vog`](https://github.com/ox-runtime/vog) | The GLFW and Dear ImGui window library used by the simulator GUI. |
| [`pyopenxr_examples`](https://github.com/ox-runtime/pyopenxr_examples) | Small OpenXR programs that double as smoke tests. |
| [`blender-xr-regression-tests`](https://github.com/cmdr2/blender-xr-regression-tests) | Unofficial end-to-end tests for Blender's XR mode, using ox. |

## Why this exists

The [FAQ](../docs/faq.md) explains the reasoning. The short version:

- **Monado** can do automated testing with a remote driver, but it mostly targets Linux. Ports to other platforms are experimental. ox targets Windows, Linux, and macOS from the start.
- **Meta Simulator** only works with Meta devices, is not open source, and is not designed for arbitrary programmatic control. ox is open source, simulates several vendors, and has a C API and HTTP API.
- Existing options are either heavy or tied to one vendor. ox keeps each part small and easy to read.

The long-term goal is a fully open-source, cross-platform XR stack that also runs on older computers.

## What to read next

- [OpenXR primer](openxr-primer.md) for the OpenXR words used in the rest of the KB.
- [Architecture](architecture.md) for how the processes and repos connect.
- [Testing an application](testing.md) if you want to use ox instead of change it.
