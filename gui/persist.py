import ctypes

from config import NUM_BUTTONS, NUM_ANALOGS, NUM_MACROS, MACRO_BYTES


class Struct(ctypes.Structure):
    def __str__(self):
        ret = ""

        def append_value(val, indent=True):
            ret = ""

            if isinstance(val, Struct):
                val = str(val)
                ret += "\n"
                for line in val.split("\n"):
                    ret += ("  " if indent else "") + line + "\n"
            elif hasattr(val, "__len__"):
                for i in val:
                    ret += "\n-"
                    lines = append_value(i, False).strip("\n").split("\n")
                    for n, line in enumerate(lines):
                        ret += (" " if n == 0 else "  ") + line
                        if n != len(lines) - 1:
                            ret += "\n"
                ret += "\n"
            else:
                ret += (" " if indent else "") + str(val) + "\n"

            return ret

        for name, type_ in self._fields_:
            ret += name + ":"
            ret += append_value(getattr(self, name))
        return ret.strip("\n")


class LedModeInfo(Struct):
    _fields_ = [
        ("lasers", ctypes.c_uint8),
    ]


class CHSV(Struct):
    _fields_ = [
        ("h", ctypes.c_uint8),
        ("s", ctypes.c_uint8),
        ("v", ctypes.c_uint8),
    ]


class CRGB(Struct):
    _fields_ = [
        ("r", ctypes.c_uint8),
        ("g", ctypes.c_uint8),
        ("b", ctypes.c_uint8),
    ]


class LargeMacro(Struct):
    _fields_ = [
        ("delay", ctypes.c_uint8),
        ("keys", ctypes.c_char * 32),
    ]


class ShortMacro(Struct):
    _fields_ = [
        ("delay", ctypes.c_uint8),
        ("keys", ctypes.c_uint8 * 10),
    ]


class AnalogConfig(Struct):
    _fields_ = [
        ("deadzone", ctypes.c_uint16),
        ("deadzone_bounceback", ctypes.c_uint8),
        ("bounceback_timer", ctypes.c_uint16),
    ]


class MacroConfig(Struct):
    _fields_ = [
        ("macro_addresses", ctypes.c_uint8 * NUM_MACROS),
        ("data", ctypes.c_uint8 * MACRO_BYTES),
    ]


class MintyConfig(Struct):
    _fields_ = [
        ("mask", ctypes.c_uint16),
        ("rainbow_on", ctypes.c_uint16),
        ("rainbow_off", ctypes.c_uint16),
        ("colours_on", CRGB * NUM_BUTTONS),
        ("colours_off", CRGB * NUM_BUTTONS),
    ]


class ConInfo(Struct):
    _fields_ = [
        ("led_mode", LedModeInfo),
        ("auto_hid", ctypes.c_bool),
        ("reactive_buttons", ctypes.c_bool),
        ("con_mode", ctypes.c_uint8),
        ("keymap", ctypes.c_char * NUM_BUTTONS),
        ("gamepad_map", ctypes.c_uint8 * NUM_BUTTONS),

        ("button_lights", ctypes.c_uint8),
        ("zone_colours", CHSV * 6),
        ("saturation", ctypes.c_uint8),
        ("zone_modes", ctypes.c_uint8 * 6),
        ("minty_config", MintyConfig),

        ("macro_layer", ctypes.c_uint8 * NUM_BUTTONS),

        ("led_dim", ctypes.c_uint16),
        ("led_timeout", ctypes.c_uint16),
        ("led_brightness", ctypes.c_uint8),

        ("analogs", AnalogConfig * NUM_ANALOGS),

        ("macros", MacroConfig),
    ]
