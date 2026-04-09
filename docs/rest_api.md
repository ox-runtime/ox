# REST API

The simulator exposes a local REST API for automation and inspection.

- URL: `http://127.0.0.1:8765`

## Session Status

```http
GET /v1/status
```

Response:

```json
{
  "session_state": "focused",
  "session_state_id": 5,
  "session_active": true,
  "fps": 72
}
```

Response fields:

- `session_state`: human-readable session state
- `session_state_id`: numeric `XrSessionState` value
- `session_active`: `true` when the session is synchronized, visible, or focused
- `fps`: app frame rate, or `0` when the session is inactive

Response codes:

- `200`: status returned
- `503`: simulator state unavailable

## Eye Textures

```http
GET /v1/views/{eye}
```

Path parameters:

- `eye`: eye index, `0` for left and `1` for right

Query parameters:

- `size` (optional): target output width in pixels; height is scaled to preserve aspect ratio

Examples:

```http
GET /v1/views/0
GET /v1/views/1
GET /v1/views/0?size=128
GET /v1/views/1?size=512
```

Response:

- Content type: `image/png`
- Body: PNG image data

Response codes:

- `200`: PNG image returned
- `404`: no frame available yet for that eye
- `503`: frame preview unavailable

## Device Profile

```http
GET /v1/profile
PUT /v1/profile
```

`GET /v1/profile` returns the active simulated device profile, including the exposed devices and their input components.

Example response:

```json
{
  "id": "oculus_quest_2",
  "name": "Meta Quest 2 (Simulated)",
  "manufacturer": "Meta Platforms",
  "interaction_profile": "/interaction_profiles/oculus/touch_controller",
  "devices": [
    {
      "user_path": "/user/head",
      "role": "hmd",
      "always_active": true,
      "components": []
    },
    {
      "user_path": "/user/hand/left",
      "role": "left_controller",
      "always_active": false,
      "components": [
        {
          "path": "/input/trigger/value",
          "type": "float",
          "description": "Trigger"
        }
      ]
    }
  ]
}
```

`PUT /v1/profile` switches the active device profile.

Request body:

```json
{
  "profile_id": "htc_vive"
}
```

Example response:

```json
{
  "status": "ok",
  "profile_id": "htc_vive",
  "name": "HTC Vive (Simulated)",
  "interaction_profile": "/interaction_profiles/htc/vive_controller"
}
```

Supported profile IDs:

- `oculus_quest_2`
- `oculus_quest_3`
- `htc_vive`
- `valve_index`
- `htc_vive_tracker`

Response codes:

- `200`: profile returned or updated
- `400`: invalid JSON or missing `device`
- `404`: unknown device profile
- `500`: no profile loaded or failed to load the selected profile

## Device State

```http
GET /v1/devices/{userPath}
PUT /v1/devices/{userPath}
```

Use the `user_path` values returned by `GET /v1/profile`, without the leading slash.

Example:

```http
GET /v1/devices/user/hand/left
```

Example response:

```json
{
  "active": true,
  "position": {"x": -0.2, "y": 1.4, "z": -0.3},
  "orientation": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0}
}
```

Update request body:

```json
{
  "position": {"x": -0.2, "y": 1.4, "z": -0.3},
  "orientation": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0},
  "active": true
}
```

Response codes:

- `200`: device updated
- `400`: invalid JSON or missing pose fields
- `404`: device not found
- `500`: failed to set device pose

## Input State

```http
GET /v1/inputs/{bindingPath}
PUT /v1/inputs/{bindingPath}
```

`bindingPath` is the user path plus the input component path, without the leading slash.

Example:

```http
GET /v1/inputs/user/hand/left/input/trigger/value
```

Example response:

```json
{
  "type": "float",
  "description": "Trigger",
  "value": 0.8
}
```

Possible values for `type`: `boolean`, `float`, or `vector2`.

Update requests must match the component type returned by `GET /v1/profile`:

**float**:
```json
{
  "value": 0.8
}
```

**boolean**:
```json
{
  "value": true
}
```

**vector2**:
```json
{
  "x": 0.25,
  "y": -0.5
}
```

Response codes:

- `200`: input updated
- `400`: invalid binding path, invalid JSON, or unsupported payload
