# Mirrored from https://github.com/ox-runtime/ox-sim-driver/blob/main/wrappers/python/ox_sim.py

import os
import sys
import ctypes
from ctypes import *
from pathlib import Path


def _load_library():
    runtime_path = _get_openxr_runtime()
    if runtime_path and "ox_runtime.json" in runtime_path:
        ox_path = os.path.dirname(runtime_path)
    else:
        raise RuntimeError(
            "Couldn't find an active ox runtime! Please set the XR_RUNTIME_JSON environment variable to /path/to/ox/ox_runtime.json."
        )

    if os.getenv("OX_USE_SIMULATOR") is None:
        raise RuntimeError("Please set the OX_USE_SIMULATOR environment variable to 1, in order to use ox_sim.py.")

    base = Path(ox_path) / "drivers" / "simulator"

    names = {
        "win32": "ox_driver.dll",
        "linux": "libox_driver.so",
        "darwin": "libox_driver.dylib",
    }

    name = names.get(sys.platform)
    if not name:
        raise RuntimeError(f"Unsupported platform: {sys.platform}")

    path = base / name
    if not path.exists():
        raise RuntimeError(f"Library not found: {path}")

    return ctypes.CDLL(str(path))


def _get_openxr_runtime():
    """
    Returns the path (or None) to the active OpenXR runtime JSON manifest.
    """
    import platform

    env_override = os.environ.get("XR_RUNTIME_JSON")
    if env_override:
        return str(Path(env_override).resolve())

    current_os = platform.system()

    if current_os == "Linux":
        # Check user-space config first
        xdg = os.environ.get("XDG_CONFIG_HOME")
        if xdg:
            config_dir = Path(xdg)
        else:
            home = os.environ.get("HOME")
            config_dir = Path(home) / ".config" if home else None

        if config_dir:
            user_link = config_dir / "openxr" / "1" / "active_runtime.json"
            if user_link.is_symlink() or user_link.exists():
                return str(user_link.resolve())

        # Fallback to system-wide Linux OpenXR location
        system_link = Path("/etc/openxr/1/active_runtime.json")
        if system_link.is_symlink() or system_link.exists():
            return str(system_link.resolve())

    elif current_os == "Darwin":
        mac_link = Path("/usr/local/share/openxr/1/active_runtime.json")
        if mac_link.is_symlink() or mac_link.exists():
            return str(mac_link.resolve())

    elif current_os == "Windows":
        import winreg

        reg_path = r"SOFTWARE\Khronos\OpenXR\1"

        # Check HKEY_LOCAL_MACHINE (System wide)
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, reg_path) as key:
                value, _ = winreg.QueryValueEx(key, "ActiveRuntime")
                return str(Path(value).resolve())
        except FileNotFoundError:
            pass

        # Check HKEY_CURRENT_USER (User override)
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, reg_path) as key:
                value, _ = winreg.QueryValueEx(key, "ActiveRuntime")
                return str(Path(value).resolve())
        except FileNotFoundError:
            pass


_lib = _load_library()


# Errors
class OxSimError(Exception):
    pass


class OxSimInvalidArgumentError(OxSimError):
    pass


class OxSimNotInitializedError(OxSimError):
    pass


class OxSimBufferTooSmallError(OxSimError):
    pass


class OxSimProfileNotFoundError(OxSimError):
    pass


class OxSimDeviceNotFoundError(OxSimError):
    pass


class OxSimComponentNotFoundError(OxSimError):
    pass


_ERROR_TYPES = {
    1: OxSimInvalidArgumentError,
    2: OxSimNotInitializedError,
    3: OxSimBufferTooSmallError,
    4: OxSimProfileNotFoundError,
    5: OxSimDeviceNotFoundError,
    6: OxSimComponentNotFoundError,
}


def _check(res):
    if res != 0:
        error_type = _ERROR_TYPES.get(res, OxSimError)
        raise error_type(f"OxSim error: {res}")


# Structs
class OxSimStatus(Structure):
    _fields_ = [
        ("session_state", c_int),
        ("session_active", c_uint32),
        ("fps", c_uint32),
    ]


class OxSimProfileInfo(Structure):
    _fields_ = [
        ("name", c_char * 128),
        ("manufacturer", c_char * 128),
        ("interaction_profile", c_char * 256),
    ]


class OxSimDeviceInfo(Structure):
    _fields_ = [
        ("user_path", c_char * 256),
        ("role", c_char * 64),
        ("always_active", c_uint32),
    ]


class OxSimComponentInfo(Structure):
    _fields_ = [
        ("path", c_char * 256),
        ("type", c_int),
        ("description", c_char * 128),
    ]


class OxSimViewInfo(Structure):
    _fields_ = [
        ("data_size", c_uint32),
        ("width", c_uint32),
        ("height", c_uint32),
        ("frame_time", c_uint64),
    ]


class XrVector2f(Structure):
    _fields_ = [("x", c_float), ("y", c_float)]


class XrVector3f(Structure):
    _fields_ = [("x", c_float), ("y", c_float), ("z", c_float)]


class XrQuaternionf(Structure):
    _fields_ = [("x", c_float), ("y", c_float), ("z", c_float), ("w", c_float)]


class XrPosef(Structure):
    _fields_ = [
        ("orientation", XrQuaternionf),
        ("position", XrVector3f),
    ]


class OxDeviceState(Structure):
    _fields_ = [
        ("user_path", c_char * 256),
        ("pose", XrPosef),
        ("is_active", c_uint32),
    ]


# Function signatures (synced with ox_sim.h)
_lib.ox_sim_initialize.restype = c_int

_lib.ox_sim_shutdown.restype = None

_lib.ox_sim_get_status.argtypes = [POINTER(OxSimStatus)]
_lib.ox_sim_get_status.restype = c_int

_lib.ox_sim_get_view_count.argtypes = [POINTER(c_uint32)]
_lib.ox_sim_get_view_count.restype = c_int

_lib.ox_sim_get_view_info.argtypes = [c_uint32, POINTER(OxSimViewInfo)]
_lib.ox_sim_get_view_info.restype = c_int

_lib.ox_sim_get_view.argtypes = [c_uint32, c_void_p, c_uint32]
_lib.ox_sim_get_view.restype = c_int

_lib.ox_sim_get_profile.argtypes = [c_char_p, c_uint32]
_lib.ox_sim_get_profile.restype = c_int

_lib.ox_sim_set_profile.argtypes = [c_char_p]
_lib.ox_sim_set_profile.restype = c_int

_lib.ox_sim_get_profile_info.argtypes = [POINTER(OxSimProfileInfo)]
_lib.ox_sim_get_profile_info.restype = c_int

_lib.ox_sim_get_device_count.argtypes = [POINTER(c_uint32)]
_lib.ox_sim_get_device_count.restype = c_int

_lib.ox_sim_get_device_info.argtypes = [c_uint32, POINTER(OxSimDeviceInfo)]
_lib.ox_sim_get_device_info.restype = c_int

_lib.ox_sim_get_device.argtypes = [c_char_p, POINTER(OxDeviceState)]
_lib.ox_sim_get_device.restype = c_int

_lib.ox_sim_set_device.argtypes = [c_char_p, POINTER(OxDeviceState)]
_lib.ox_sim_set_device.restype = c_int

_lib.ox_sim_get_component_count.argtypes = [c_char_p, POINTER(c_uint32)]
_lib.ox_sim_get_component_count.restype = c_int

_lib.ox_sim_get_component_info.argtypes = [c_char_p, c_uint32, POINTER(OxSimComponentInfo)]
_lib.ox_sim_get_component_info.restype = c_int

_lib.ox_sim_get_input_boolean.argtypes = [c_char_p, c_char_p, POINTER(c_uint32)]
_lib.ox_sim_get_input_boolean.restype = c_int

_lib.ox_sim_set_input_boolean.argtypes = [c_char_p, c_char_p, c_uint32]
_lib.ox_sim_set_input_boolean.restype = c_int

_lib.ox_sim_get_input_float.argtypes = [c_char_p, c_char_p, POINTER(c_float)]
_lib.ox_sim_get_input_float.restype = c_int

_lib.ox_sim_set_input_float.argtypes = [c_char_p, c_char_p, c_float]
_lib.ox_sim_set_input_float.restype = c_int

_lib.ox_sim_get_input_vector2f.argtypes = [c_char_p, c_char_p, POINTER(XrVector2f)]
_lib.ox_sim_get_input_vector2f.restype = c_int

_lib.ox_sim_set_input_vector2f.argtypes = [c_char_p, c_char_p, POINTER(XrVector2f)]
_lib.ox_sim_set_input_vector2f.restype = c_int


# Simulator
class Simulator:
    __slots__ = ["_device_cache"]

    def __init__(self):
        _check(_lib.ox_sim_initialize())
        self._device_cache = None

    def shutdown(self):
        _lib.ox_sim_shutdown()

    @property
    def status(self):
        s = OxSimStatus()
        _check(_lib.ox_sim_get_status(byref(s)))
        return {
            "state": s.session_state,
            "active": bool(s.session_active),
            "fps": s.fps,
        }

    @property
    def profile(self):
        buf = create_string_buffer(64)
        _check(_lib.ox_sim_get_profile(buf, len(buf)))
        return buf.value.decode()

    @profile.setter
    def profile(self, value: str):
        _check(_lib.ox_sim_set_profile(value.encode()))

    @property
    def profile_info(self):
        info = OxSimProfileInfo()
        _check(_lib.ox_sim_get_profile_info(byref(info)))
        return {
            "name": info.name.decode(),
            "manufacturer": info.manufacturer.decode(),
            "interaction_profile": info.interaction_profile.decode(),
        }

    def _load_devices(self):
        if self._device_cache is not None:
            return

        self._device_cache = {}

        count = c_uint32()
        _check(_lib.ox_sim_get_device_count(byref(count)))

        for i in range(count.value):
            info = OxSimDeviceInfo()
            _check(_lib.ox_sim_get_device_info(i, byref(info)))
            d = Device(self, info)
            self._device_cache[d.user_path] = d

    def devices(self):
        self._load_devices()
        return list(self._device_cache.values())

    def device(self, user_path: str):
        self._load_devices()
        try:
            return self._device_cache[user_path]
        except KeyError as exc:
            raise OxSimDeviceNotFoundError(f"Device not found: {user_path}") from exc

    def views(self):
        count = c_uint32()
        _check(_lib.ox_sim_get_view_count(byref(count)))
        return [View(self, i) for i in range(count.value)]


# Device
class Device:
    __slots__ = ["sim", "user_path", "role", "always_active", "_components"]

    def __init__(self, sim, info: OxSimDeviceInfo):
        self.sim = sim
        self.user_path = info.user_path.decode()
        self.role = info.role.decode()
        self.always_active = bool(info.always_active)
        self._components = None

    # Pose
    def _get_state(self):
        s = OxDeviceState()
        _check(_lib.ox_sim_get_device(self.user_path.encode(), byref(s)))
        return s

    def _set_state(self, position=None, orientation=None, active=None):
        s = self._get_state()

        if position is not None:
            s.pose.position.x = position[0]
            s.pose.position.y = position[1]
            s.pose.position.z = position[2]

        if orientation is not None:
            s.pose.orientation.x = orientation[0]
            s.pose.orientation.y = orientation[1]
            s.pose.orientation.z = orientation[2]
            s.pose.orientation.w = orientation[3]

        if active is not None:
            s.is_active = int(active)

        _check(_lib.ox_sim_set_device(self.user_path.encode(), byref(s)))

    @property
    def position(self):
        position = self._get_state().pose.position
        return (position.x, position.y, position.z)

    @position.setter
    def position(self, value):
        self._set_state(position=value)

    @property
    def orientation(self):
        orientation = self._get_state().pose.orientation
        return (orientation.x, orientation.y, orientation.z, orientation.w)

    @orientation.setter
    def orientation(self, value):
        self._set_state(orientation=value)

    @property
    def active(self):
        return bool(self._get_state().is_active)

    @active.setter
    def active(self, value):
        self._set_state(active=value)

    # Inputs
    def _load_components(self):
        if self._components is not None:
            return

        count = c_uint32()
        _check(_lib.ox_sim_get_component_count(self.user_path.encode(), byref(count)))

        self._components = {}

        for i in range(count.value):
            info = OxSimComponentInfo()
            _check(_lib.ox_sim_get_component_info(self.user_path.encode(), i, byref(info)))
            self._components[info.path.decode()] = info.type

    def _get_type(self, path):
        self._load_components()
        if path not in self._components:
            raise OxSimComponentNotFoundError(f"Component not found: {path}")
        return self._components[path]

    def set_input(self, path, value):
        t = self._get_type(path)

        up = self.user_path.encode()
        p = path.encode()

        if t == 0:
            _check(_lib.ox_sim_set_input_boolean(up, p, int(value)))
        elif t == 1:
            _check(_lib.ox_sim_set_input_float(up, p, c_float(value)))
        elif t == 2:
            v = XrVector2f(value[0], value[1])
            _check(_lib.ox_sim_set_input_vector2f(up, p, byref(v)))

    def get_input(self, path):
        t = self._get_type(path)

        up = self.user_path.encode()
        p = path.encode()

        if t == 0:
            v = c_uint32()
            _check(_lib.ox_sim_get_input_boolean(up, p, byref(v)))
            return bool(v.value)

        elif t == 1:
            v = c_float()
            _check(_lib.ox_sim_get_input_float(up, p, byref(v)))
            return v.value

        elif t == 2:
            v = XrVector2f()
            _check(_lib.ox_sim_get_input_vector2f(up, p, byref(v)))
            return (v.x, v.y)


# View
class View:
    __slots__ = ["sim", "index"]

    def __init__(self, sim, index):
        self.sim = sim
        self.index = index

    def info(self):
        vi = OxSimViewInfo()
        _check(_lib.ox_sim_get_view_info(self.index, byref(vi)))
        return vi

    def image(self):
        vi = self.info()
        buf = (c_uint8 * vi.data_size)()
        _check(_lib.ox_sim_get_view(self.index, buf, vi.data_size))
        return bytes(buf), vi.width, vi.height
