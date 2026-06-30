# ox

**ox** is a new lightweight and cross-platform OpenXR runtime for Windows, Linux and Mac. It supports OpenGL, Vulkan and Metal.

The primary purpose of ox is automated testing of OpenXR applications using a virtual OpenXR device. It is useful during development and CI testing.

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
Headline releases only. See the [releases](https://github.com/ox-runtime/ox/releases) page for fine-grained releases.

- 30 Jun 2026 - v0.7 - Regression test suite for Blender's XR API. [[more...](https://github.com/cmdr2/blender-xr-regression-tests)]
- 21 May 2026 - v0.7 - Python wrapper for in-process testing. [[more...](docs/automated_testing.md)]
- 9 Apr 2026 - v0.6 - In-Process Simulator C-API. Redesigned GUI. Additional release formats: dmg, Windows Installer, AppImage, flatpak and snap.
- 10 Mar 2026 - v0.5 - Metal support. Live preview in the Simulator GUI window.
- 31 Jan 2026 - v0.4 - Vulkan support (versions 1.0 to 1.3).
- 28 Jan 2026 - v0.3 - First release of ox with a simulator GUI and REST API. Supports OpenGL on Windows, Linux and macOS.

## [Development](docs/development.md)

## [Documentation](docs/README.md)

## [FAQ](docs/faq.md)

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
- [OpenXR Loader Design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md)
