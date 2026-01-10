# ox runtime architecture

## Overview

The **ox** runtime has been split into a **client-service architecture** to allow the service to remain running even if the application crashes. This provides better stability and allows for centralized management of hardware, lens distortion, and compositor operations.

## Architecture Components

### 1. Protocol Layer (`src/protocol/`)

Zero-overhead IPC layer designed for sub-microsecond latency:

- **`shared_memory.h`**: Cross-platform shared memory wrapper
  - Windows: `CreateFileMapping` / `MapViewOfFile`
  - Linux: `shm_open` / `mmap`
  - Zero-copy, direct memory access

- **`control_channel.h`**: Control message channel
  - Windows: Named Pipes (`\\.\pipe\ox_runtime_control`)
  - Linux: Unix Domain Sockets (`/tmp/ox_runtime_control.sock`)
  - Used for lifecycle events (connect, disconnect, session creation)

- **`messages.h`**: Protocol definitions
  - Cache-line aligned structs (64-byte alignment)
  - Atomic operations for lock-free access
  - Platform-agnostic handle sharing

- **`util.h`**: Platform utilities
  - Windows security descriptor creation for shared objects
  - Owner-only access control for IPC resources

### 2. Client Library (`src/client/`)

Thin DLL that implements the OpenXR API:

- **`runtime.cpp`**: OpenXR API implementation
  - All `xr*` functions
  - Function dispatch table
  - Minimal overhead wrapper

- **`service_connection.cpp`**: Service connection management
  - Connects to running service
  - Maintains connection to shared memory
  - Handles control message exchange with service

### 3. Service Process (`src/service/`)

Independent background process:

- **`main.cpp`**: Service implementation
  - Runs at approximately 90Hz generating mock tracking data
  - Maintains shared memory state
  - Supports multiple client connections sequentially
  - Handles control messages for session lifecycle
  - Future: compositor, lens distortion, hardware I/O

## Data Flow

### Hot Path (Tracking Data)
```
Service (90Hz loop)
    ↓ atomic store (~10ns)
[Shared Memory - FrameState]
    ↑ atomic load (~10ns)
Client (per frame)
```

**Latency Budget**: <100 nanoseconds per operation

### Cold Path (Control Messages)
```
Client → Named Pipe/Socket → Service
         ↑ Response ↓
Client ← Named Pipe/Socket ← Service
```

**Latency**: Sub-millisecond (adequate for lifecycle events)

## Shared Memory Layout

```c++
struct SharedData {
    atomic<uint32_t> protocol_version;  // Protocol versioning
    atomic<uint32_t> service_ready;     // Service state
    atomic<uint32_t> client_connected;  // Client state
    
    FrameState frame_state;             // HOT: 90Hz updates
    // Contains:
    //   - frame_id (atomic)
    //   - predicted_display_time (atomic)
    //   - View poses [2] (stereo)
    //   - FOV data
    //   - Graphics handles (platform-specific)
};
```

**Total size**: 4KB (fits in single memory page)

## Platform Compatibility

### Windows
- Shared Memory: File Mapping API (`CreateFileMappingA` with name `ox_runtime_shm`)
- Control Channel: Named Pipes (`\\.\pipe\ox_runtime_control`)
- Graphics Handles: NT handles (uint64_t)
- Service Launch: `CreateProcess` with `CREATE_NO_WINDOW` (future)
- Security: SDDL-based security descriptors for owner-only access

### Linux
- Shared Memory: POSIX shared memory (`shm_open` with name `ox_runtime_shm`)
- Control Channel: Unix Domain Sockets (`/tmp/ox_runtime_control.sock`)
- Graphics Handles: DMA-BUF file descriptors (int32_t)
- Service Launch: `fork` + `exec` (future)
- Security: 0600 permissions for owner-only access

### macOS/Android (Future)
- Same POSIX APIs as Linux
- May need different graphics handle mechanisms

## Performance Characteristics

### Memory Operations
- Atomic load/store: ~10-50 nanoseconds
- Cache line alignment prevents false sharing
- Lock-free for entire hot path

### Context Switches
- Service runs independently (no blocking client)
- Client never blocks on service (reads atomic state)
- Control channel only for infrequent operations

### Startup
1. Client checks for service (~1ms)
2. If not running, service must be started manually (~500ms startup time)
3. Client connects to shared memory (`ox_runtime_shm`, ~1ms)
4. Client connects to control channel (~1ms)
5. Verifies protocol version
6. Sends CONNECT message to service
7. Ready to render

## Future Enhancements

1. **Graphics Handle Sharing**
   - Windows: D3D11/D3D12 NT handle sharing
   - Linux: Vulkan + DMA-BUF external memory

2. **True Multiple Concurrent Clients**
   - Service manages multiple shared memory regions simultaneously
   - Per-client frame pacing
   - Currently supports multiple clients sequentially only

3. **Compositor Integration**
   - Service owns swapchain
   - Lens distortion in service
   - Reprojection on service thread

4. **Hardware Integration**
   - Service talks directly to HMD
   - IMU fusion
   - Display synchronization
   - Real tracking data (currently mock data only)

5. **Crash Recovery**
   - Service detects client crashes
   - Cleans up resources
   - Ready for next client
