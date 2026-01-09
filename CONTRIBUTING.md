**ox** is a fully open-source, cross-platform OpenXR runtime. Please feel free to contribute fixes, documentation and testing reports!

**ox** intends to be easy to develop for, which means it doesn't require a complicated environment setup, and has a simple, modular architecture.

## Architecture

### Key Components

1. **xrNegotiateLoaderRuntimeInterface** - Entry point for OpenXR loader
   - Validates loader/runtime version compatibility
   - Provides function dispatch table via `xrGetInstanceProcAddr`

2. **xrGetInstanceProcAddr** - Function pointer dispatcher
   - Maps function names to implementations
   - Required for all OpenXR function lookups

3. **Instance Management**
   - `xrCreateInstance` - Creates instance handles
   - `xrDestroyInstance` - Cleans up instances
   - Simple handle generation using incrementing integers

4. **Runtime Information**
   - `xrGetInstanceProperties` - Returns runtime name/version
   - Reports as "ox" version 1.0.0

## Implemented Functions

| Function | Purpose | Implementation |
|----------|---------|----------------|
| `xrNegotiateLoaderRuntimeInterface` | Loader negotiation | Full |
| `xrGetInstanceProcAddr` | Function dispatch | Full |
| `xrEnumerateApiLayerProperties` | List API layers | Returns 0 layers |
| `xrEnumerateInstanceExtensionProperties` | List extensions | Returns 0 extensions |
| `xrCreateInstance` | Create instance | Full |
| `xrDestroyInstance` | Destroy instance | Full |
| `xrGetInstanceProperties` | Get runtime info | Full |
| `xrPollEvent` | Poll for events | Returns no events |
| `xrResultToString` | Convert result to string | Basic implementation |
| `xrStructureTypeToString` | Convert type to string | Basic implementation |

## Limitations

This is a minimal runtime designed (currently) for automated testing. It does **NOT**:
- ❌ Support any VR/AR hardware
- ❌ Implement session management
- ❌ Support spaces or actions
- ❌ Implement rendering
- ❌ Support any extensions
- ❌ Provide actual VR/AR functionality

## Extending This Runtime

To add more functionality, you would typically implement:
1. Session management (`xrCreateSession`, `xrDestroySession`)
2. Reference spaces (`xrCreateReferenceSpace`)
3. Swapchains for rendering (`xrCreateSwapchain`)
4. Action system for input (`xrCreateActionSet`, `xrSyncActions`)
5. Frame timing (`xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`)
6. Hardware integration (headset tracking, controllers, etc.)
