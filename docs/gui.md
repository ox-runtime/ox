# ox-simulator GUI Reference

See the headset's view and control the devices visually in a GUI window. Useful for development and live testing.

<img height="400" alt="image" src="https://github.com/user-attachments/assets/e2b888d6-2295-4aa7-8aa0-be7f1b620c08" />

## Usage

Run `ox.exe` (Windows) or `./ox` (Linux/macOS) to launch the simulator GUI.

## Overview

The simulator GUI is divided into three areas:

- **Top toolbar** - runtime registration, API server controls, and simulated device selection
- **Preview panel** (left) - live eye texture feed from the connected OpenXR application
- **Sidebar** (right) - per-device pose and input controls; drag the splitter to resize

---

## Top Toolbar

The top toolbar contains the global controls that affect the whole simulator session.

| Control | Description |
|--------|-------------|
| **Set as OpenXR Runtime** | Registers ox as the active OpenXR runtime for the current machine. On Windows this prompts for elevation; on Linux and macOS it updates the user-level OpenXR runtime link. |
| **Copy runtime path** | Copies the path to the generated `ox_runtime.json` manifest. This is the value to use for the `XR_RUNTIME_JSON` environment variable when you want to select ox without system-wide registration. |
| **API Server** toggle | Starts or stops the local HTTP API server on port `8765`. |
| **Copy API URL** | Copies the API base URL, `http://127.0.0.1:8765`, to the clipboard. |
| **Simulated Device** | Switches the active device profile, for example Quest 2, Quest 3, Vive, Index, or Vive Tracker. |

---

## Preview Panel

The preview panel displays the rendered frames submitted by the OpenXR application.

### View selector

A combo box in the top-right of the preview toolbar chooses which eye(s) to display:

| Option | Description |
|--------|-------------|
| Left   | Left-eye texture only |
| Right  | Right-eye texture only |
| Both   | Left and right textures side-by-side |

### Session indicator

The top-left of the preview toolbar shows whether an OpenXR session is currently active.

### Copy to clipboard

Click the copy button overlaid in the top-right corner of the preview image to copy the current eye texture view as an image. In **Both** mode a side-by-side composite is copied.

---

## Head Pose Navigation

Click anywhere inside the preview image to give it keyboard/mouse focus. This will allow you to control the head pose with the mouse and keyboard.

### Mouse

| Action | Effect |
|--------|--------|
| Left-drag on image | FPS mouse-look: yaw (horizontal) and pitch (vertical); roll is never introduced |

### Keyboard - Translation

Movement uses an FPS-style basis. `W`/`A`/`S`/`D` follow the viewer's yaw only, so looking up/down or rolling does not tilt movement into the air. `R`/`F` always move along world up/down.

| Key | Action |
|-----|--------|
| `W` | Move forward along the current yaw direction |
| `S` | Move backward along the current yaw direction |
| `A` | Strafe left relative to the current yaw |
| `D` | Strafe right relative to the current yaw |
| `R` | Move up along world up |
| `F` | Move down along world up |
| `Left Shift` | Hold to move at 3× speed (works with all movement keys) |

### Keyboard - Rotation

Arrow-key pitch/yaw matches mouse-look and ignores the viewer's current roll. `Q` and `E` still roll around the current view forward axis.

| Key | Action |
|-----|--------|
| `↑` Up arrow    | Pitch up around the yaw-aligned right axis |
| `↓` Down arrow  | Pitch down around the yaw-aligned right axis |
| `←` Left arrow  | Yaw left around world up |
| `→` Right arrow | Yaw right around world up |
| `Q` | Roll left around the view forward axis |
| `E` | Roll right around the view forward axis |

---

## Sidebar - Device Panels

Each device (HMD, controllers, trackers) has its own panel in the sidebar. Scroll down to see all devices.

### Pose controls

| Control | Description |
|---------|-------------|
| **Active** toggle | Enable or disable tracking for this device |
| **Position** drag (X Y Z) | Drag each axis or Double-click to type a value |
| **Rotation** drag (Pitch Yaw Roll, °) | Absolute Euler editor; setting all three axes back to `0` restores the identity orientation |
| **Reset Pose** | Restore the device to its default pose |

### Input components

Boolean (button) inputs are shown as toggle switches. Float inputs are shown as sliders.

Trigger and squeeze-style float inputs also include a small filled-circle hold button next to the slider. Pressing and holding it ramps the value to `1.0` over `300 ms`, keeps it pinned at `1.0` while held, and releases it back to `0.0` over `100 ms` when released. Thumbstick and trackpad axes continue to use standard sliders only.

---

## Status Bar

The status bar at the bottom of the window shows transient messages (copy results, errors). It also displays hover coordinates when the mouse is over the preview image: `Cursor: Left (x, y)` / `Cursor: Right (x, y)`.
