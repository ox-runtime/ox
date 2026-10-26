# IPC proxy internals

`ox-ipc-proxy` carries driver calls between two processes. It looks like a driver on both sides: driver calls go in, the same calls come out in the other process.

The reason is safety. The application process does not load the real device driver. If that driver crashes, the host process can die without taking the application with it. The host can also expose the driver to a GUI or HTTP client while the application runs.

## Two libraries

The build makes two shared libraries:

- **`ox_ipc_client`** implements the callbacks the runtime expects. The runtime loads it when no other driver is selected.
- **`ox_ipc_server`** is loaded by the `ox` host. The host passes it the real driver with `ox_ipc_server_set_driver()`, then calls `ox_ipc_server_initialize()`.

The server exports three functions:

```c
void ox_ipc_server_set_driver(const OxDriver* driver);
int  ox_ipc_server_initialize();
void ox_ipc_server_shutdown();
```

`SetDriver` copies the callback table. `Initialize` checks that the required callbacks exist and that the device reports connected. It then creates the shared-memory region, starts the control channel, and starts two threads: the server loop and the frame loop.

## Control channel and shared memory

The proxy uses two paths, because it carries two kinds of data.

The **control channel** is an [nng](https://github.com/nanomsg/nng) `pair0` socket. On Windows the address is `ipc://ox_ipc_proxy_control`. On POSIX it is `ipc:///tmp/ox_ipc_proxy_control.ipc`. Messages are defined in [`messages.h`](https://github.com/ox-runtime/ox-ipc-proxy/blob/main/src/messages.h):

| Message | Direction | Purpose |
|---|---|---|
| `CONNECT` | client to server | Handshake. Sets the client-connected flag. |
| `DISCONNECT` | client to server | Clears the flag and lets the server clean up. |
| `GET_SYSTEM_PROPERTIES` | client to server | Fetch the system name, vendor, limits, and tracking flags. |
| `GET_INTERACTION_PROFILES` | client to server | Fetch the driver's interaction profiles. |
| `REGISTER_INPUT` | client to server | Reserve a shared-memory slot for an input path. |
| `NOTIFY_SESSION_STATE` | client to server | Tell the driver about a session state change. |
| `RESPONSE` | server to client | A reply, matched by sequence number. |

Each message starts with a `MessageHeader` that has the type, sequence number, payload size, and a reserved field. Replies use the request's sequence number. The protocol version is `1`.

The **shared memory** carries the data that changes every frame: poses, device states, input values, and eye images. The client reads and writes it directly, so there is no syscall per frame. The region is named `/ox_ipc_proxy_shm`. Its layout is in [`shared_memory.h`](https://github.com/ox-runtime/ox-ipc-proxy/blob/main/src/shared_memory.h).

## Shared memory layout

`SharedData` starts with protocol fields and a server-ready flag. After that come two large sections:

- `FrameState`: the predicted display time, two view states, up to `OX_MAX_DEVICES` device states, and two `FrameTexture` buffers.
- `InputStateTable`: up to 64 input slots. Each slot has a user path, component path, type, availability flag, and a value (boolean, float, or vec2).

Poses and device states use a sequence counter to avoid torn reads. The writer makes the counter odd, writes the data, then makes it even. The reader retries until the counter is the same even number before and after the read. No mutex is needed.

Each `FrameTexture` holds up to 2048 by 2048 RGBA pixels, plus width, height, format, data size, and a ready flag. That is about 16 MB per eye, so the whole region is tens of megabytes. The server clears it when it is created.

## What the client does

[`ipc_client.cpp`](https://github.com/ox-runtime/ox-ipc-proxy/blob/main/src/ipc_client.cpp) maps the shared region, checks the protocol version, opens the control socket, sends `CONNECT`, and fetches the system properties and interaction profiles. If any step fails, it cleans up and reports disconnected.

Most client callbacks are reads:

- `update_view` reads the pose for the requested eye and copies the field of view.
- `update_devices` reads the device count and snapshots each device.
- `get_input_state_*` finds or registers an input slot, then reads its value.
- `get_system_properties` returns the metadata cached during the handshake.

The client only writes in two places. `submit_frame_pixels` copies the RGBA buffer into the eye's `FrameTexture` and sets the ready flag. `NotifySessionState` sends a control message.

A mutex serializes the public calls, so the socket and shared pointers are not used at the same time.

## What the server does

[`ipc_server.cpp`](https://github.com/ox-runtime/ox-ipc-proxy/blob/main/src/ipc_server.cpp) creates the shared region and listens on the control channel. The server loop handles control messages and metadata requests.

The frame loop wakes about 90 times per second. When a client is connected, it:

1. Stores a predicted display time.
2. Calls the real driver's `update_view` for both eyes and writes the poses and fields of view.
3. Calls `update_devices` and writes each device state.
4. Refreshes every registered input slot from the driver.
5. For each eye whose texture is ready, clears the flag and forwards the pixels to the driver's `submit_frame_pixels`.

`HandleRegisterInput` reuses a slot if the same path and type are already registered, so the client's cache and the shared table stay in step. If the table is full, registration returns `MAX_INPUT_SLOTS`, and the client reports the path as unsupported.

## When something fails

The proxy is built so the application survives a driver failure. The application only has `ox_ipc_client` loaded. If the host or driver crashes, the client's reads fail or return unsupported paths, but the application process keeps running. The host can be restarted on its own.

There are limits. The shared region holds two eyes and 64 input slots. Textures larger than 2048 by 2048 are rejected. The protocol is versioned but only checks that both sides match, so client and server must be built from compatible sources. The server assumes one client at a time, which matches the `pair0` socket.

## Tests

[`tests`](https://github.com/ox-runtime/ox-ipc-proxy/tree/main/tests) covers frame transfer, input registration and updates, lifecycle, metadata, and session notification. The tests use a fake driver, so they do not need a GPU or the simulator.

## Related pages

- [Architecture](architecture.md) for the process diagrams and the driver order.
- [Runtime](runtime.md) for how `ox_ipc_client` is selected.
- [Simulator](simulator.md) for the driver on the other side.
- [ox-ipc-proxy README](https://github.com/ox-runtime/ox-ipc-proxy#readme)
