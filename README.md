# ox

**ox** is a simple and lightweight OpenXR runtime for Windows and Linux.

The primary purpose of **ox** is automated testing of OpenXR applications. It comes with a virtual OpenXR device which can be controlled programmatically (e.g. press a button, turn the headset etc). The effect of these actions can then be verified in the OpenXR application that you're testing.

## Building

```bash
# Initialize submodules
git submodule update --init

# Build the runtime
cmake -B build
cmake --build build --config Release

# Quick test (requires https://github.com/cmbruns/pyopenxr_examples)
python tests/test.py
```

Expected output:
```bash
The current active OpenXR runtime is: ox
```

The build produces (in `./build/bin/`):
- **Windows**: `ox.dll` and `ox_openxr.json`
- **Linux**: `libox.so` and `ox_openxr.json`

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

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md).

## Troubleshooting

**"OpenXR SDK not found"**
- Initialize the submodule: `git submodule update --init`

**"XR_ERROR_RUNTIME_UNAVAILABLE"**
- Check that `XR_RUNTIME_JSON` environment variable is set correctly
- Verify the manifest file exists at `build/bin/ox_openxr.json`
- Ensure the runtime library is in the same directory as the manifest

**Build errors on Linux**
- Make sure you have OpenGL development headers installed
- Install: `sudo apt-get install libgl1-mesa-dev libx11-dev`

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
- [OpenXR Loader Design](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/loader/LoaderDesign.md)
