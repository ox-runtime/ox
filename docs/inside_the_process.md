# Inside the Process

Test your OpenXR application with a virtual XR device, using your existing testing framework.

In this configuration, the ox runtime will run inside your application process. This lets you to keep your tests focused on the application logic, without including the additional complexity of cross-process communication in your test path.

## Setup
1. Download [ox](https://github.com/ox-runtime/ox/releases).
2. Get one of the wrappers to communicate with the virtual XR device in your tests:
- Python: [ox_sim.py](https://github.com/ox-runtime/ox-sim-driver/tree/main/wrappers/python)
- C: [ox_sim.h](https://github.com/ox-runtime/ox/blob/main/docs/c_api.md)

Please see the respective wrapper documentation for writing tests in that language.

## Run the Tests
1. Set the `XR_RUNTIME_JSON` environment variable to point to the runtime manifest:

* **Windows:** `set XR_RUNTIME_JSON=C:\path\to\ox\ox_openxr.json`
* **Linux/macOS:** `export XR_RUNTIME_JSON=/path/to/ox/ox_openxr.json`

2. Set `OX_USE_SIMULATOR=1` to enable the virtual XR device:

* **Windows:** `set OX_USE_SIMULATOR=1`
* **Linux/macOS:** `export OX_USE_SIMULATOR=1`

3. Run your tests using your regular testing framework, e.g. `python -m pytest`.
