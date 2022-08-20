import ctypes


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


class LargeMacro(Struct):
    _fields_ = [
        ("delay", ctypes.c_uint8),
        ("keys", ctypes.c_char * 32),
    ]


class ShortMacro(Struct):
    _fields_ = [
        ("delay", ctypes.c_uint8),
        ("keys", ctypes.c_char * 10),
    ]


class ConInfo(Struct):
    _fields_ = [
        ("led_mode", LedModeInfo),
        ("auto_hid", ctypes.c_bool),
        ("reactive_buttons", ctypes.c_bool),
        ("con_mode", ctypes.c_uint8),
        ("keymap", ctypes.c_char * 10),

        ("button_lights", ctypes.c_uint8),
        ("zone_colours", CHSV * 6),
        ("zone_modes", ctypes.c_uint8 * 6),

        ("large_macros", LargeMacro * 2),
        ("short_macros", ShortMacro * 4),
        ("tiny_macro_speed", ctypes.c_uint8),
        ("macro_layer", ctypes.c_uint8 * 10),
    ]
