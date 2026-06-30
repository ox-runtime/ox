## Custom Drivers

ox allows you to add support for real DIY/mainstream headsets and controllers using custom drivers. This will allow you to use any OpenXR application/game with your hardware, across Windows, Linux and Mac.

This is a power-user feature, meant for developers working with real DIY/mainstream hardware.

**Note:** Don't write a driver if you just want to simulate your hardware in the simulator - add that to the simulator's list of [device profiles](https://github.com/ox-runtime/ox-sim-driver/blob/main/src/device_profiles.cpp) instead.

> [!TIP]
> Fun fact: the simulator is also just a driver for ox, called [ox-sim-driver](https://github.com/ox-runtime/ox-sim-driver). It shows the rendered pixels in a GUI window and sends your simulator inputs back as device inputs.

## Step 1: Write your Driver
A driver is a shared library that implements the [ox_driver.h](https://github.com/ox-runtime/ox-runtime/blob/main/include/ox_driver.h) interface.

Clone the [ox-driver-example](https://github.com/ox-runtime/ox-driver-example) repository to get started.

## Step 2: Install your Driver
The driver should be a folder placed in the `drivers` folder. The folder should be named after the driver, and contain the driver's shared library.

For example:
```text
drivers/
└── my_vive_driver/
    └── ox_driver.dll
```

ox expects the shared library file name to be:
* On Windows: `ox_driver.dll`
* On macOS: `ox_driver.dylib`
* On Linux: `ox_driver.so`

## Step 3: Load your Driver

1. If not already done, set ox as the active runtime (either using the [GUI](gui.md) or by setting the `XR_RUNTIME_JSON` environment variable).
2. ox currently expects only a single driver to be present in the `drivers` folder. So for now, please move out the `simulator` driver folder, and leave only your driver folder in `drivers`.
3. Start `ox`, and then run the XR application/game that you want to test with.

### Advanced Debugging

If you want to bypass IPC and load your driver directly into the application process, set the `OX_RUNTIME_DRIVER` environment variable to the name of your driver folder. For example:
```bash
export OX_RUNTIME_DRIVER=my_vive_driver
```

This will load your driver directly into the application process, and is useful for step-by-step debugging.

You don't need to run `ox` separately in this case, just run your XR application/game as usual.
