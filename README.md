# ox

**ox** is a new lightweight and cross-platform OpenXR runtime for Windows, Linux and Mac. It supports OpenGL, Vulkan and Metal.

Test OpenXR applications automatically using a virtual OpenXR device. Useful during development and CI testing.

You can control a virtual XR device programmatically (e.g. press a button, move the headset, read screen texture etc), and integrate it with your existing test framework and code.

ox simulates Meta Quest, HTC Vive Trackers, Valve Index and other popular headsets.

> [!WARNING]
> **WORK-IN-PROGRESS** - ox is still heavily under-development and is not (yet) fully compliant with the OpenXR spec.

<img height="100" alt="different ways of using ox: automated testing, GUI, REST API" src="https://github.com/user-attachments/assets/22dc60eb-cba2-4353-abf2-e09b215fca6e" />

## Get Started

Click to use ox for:
* [Automated Testing](docs/automated_testing.md) - Use virtual XR devices inside your application and test framework. Useful for automated testing (including CI runners).
* [GUI](docs/gui.md) - Preview the headset's output and control the devices visually in a GUI window. Useful for development and live testing.
* [REST API](docs/rest_api.md) - Control the virtual XR device over HTTP. Useful for agentic development, or when you can't use the in-process API.

## News
See the [releases](https://github.com/ox-runtime/ox/releases) page for more fine-grained releases.

- 2026/06/30 - v0.7 - released a regression test suite for Blender's XR API. [[more...](https://github.com/cmdr2/blender-xr-regression-tests)]
- 2026/05/21 - v0.7 - ox now has a Python wrapper for easier in-process testing. [[more...](docs/automated_testing.md)]
- 2026/04/09 - v0.6 - ox now supports XR testing fully within the application process (i.e. no IPC). Also releases a re-designed GUI, and additional release formats: dmg, Windows Installer, AppImage, flatpak and snap.
- 2026/03/10 - v0.5 - ox now supports Metal, and live preview in the Simulator GUI window.
- 2026/01/31 - v0.4 - ox now supports Vulkan (versions 1.0 to 1.3).
- 2026/01/28 - v0.3 - First public version of ox released, with a simulator GUI and REST API. Supports OpenGL on Windows, Linux and macOS.

## [Development](docs/development.md)

## [Documentation](docs/README.md)

## [FAQ](docs/faq.md)

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
- [OpenXR Loader Design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md)
