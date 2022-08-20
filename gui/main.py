import ctypes

import tkinter as tk
import tkinter.ttk as ttk
from tkinter.colorchooser import askcolor

import serial
import serial.tools.list_ports


VID = 0x1ccf
PID = 0x101c

VENDOR_NAME = "YuanCon (CFW)"
PRODUCT_NAME = "YuanCon (CFW)"
BAUDRATE = 115200

PERSIST_DATA_VERSION = 1


def find_yuan_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if port.vid != VID or port.pid != PID:
            continue

        return port.name
    return None


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
        ('lasers', ctypes.c_uint8),
        ('start', ctypes.c_uint8),
        ('wing_upper', ctypes.c_uint8),
        ('wing_lower', ctypes.c_uint8),
        ('buttons', ctypes.c_uint8),
    ]


class CHSV(Struct):
    _fields_ = [
        ('h', ctypes.c_uint8),
        ('s', ctypes.c_uint8),
        ('v', ctypes.c_uint8),
    ]


class ConInfo(Struct):
    _fields_ = [
        ('led_mode', LedModeInfo),
        ('led_solid_l', CHSV * 3),
        ('led_solid_r', CHSV * 3),
        ('auto_hid', ctypes.c_bool),
        ('reactive_buttons', ctypes.c_bool),
        ('con_mode', ctypes.c_uint8),
        ('keymap', ctypes.c_char * 10),
    ]


# @dataclass
# class ConInfo:
#     _FORMAT = "5B3B3B3B3B3B3BBBB10B"

#     led_mode: List[int]
#     led_solid_l: List[List[int]]
#     led_solid_r: List[List[int]]
#     auto_hid: bool
#     reactive_buttons: bool
#     con_mode: int
#     keymap: List[int]


class Modal(tk.Toplevel):
    def __init__(self, parent):
        super().__init__(parent)
        self.wait_visibility()
        self.grab_set()
        self.lift()
        self.protocol('WM_DELETE_WINDOW', self._close)
        self.bind("<1>", self._capture_click)

    def _capture_click(self, event):
        if event.widget == self:
            if event.x < 0 or event.x > self.winfo_width() or event.y < 0 or event.y > self.winfo_height():
                self.bell()
                self.focus()

    def _close(self):
        self.grab_release()
        self.destroy()


class Keybinder(Modal):
    FONT = ("SegoeUI", 10, "bold")

    def __init__(self, parent, name, key, callback):
        super().__init__(parent)
        self.geometry("200x100")
        self.resizable(0, 0)

        self.title("Bind key")

        kb_frame = tk.Frame(self)
        kb_frame.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

        self.bind("<KeyPress>", self._bind_key)
        self.focus_set()

        self.for_lab = tk.Label(kb_frame, text=f"Press the key for {name}:", font=self.FONT)
        self.for_lab.pack()
        self.bind_lab = tk.Label(kb_frame, text=f"<{chr(key)}>", font=self.FONT)
        self.bind_lab.pack()
        self.callback = callback

    def _bind_key(self, key):
        if key.char:
            self.bind_lab.configure(text=f"<{key.char}>")
            self.callback(ord(key.char))
            self._close()


def hsv_to_rgb(h, s, v):
    if s == 0.0:
        return (v, v, v)
    i = int(h * 6)
    f = (h * 6) - i
    p = v*(1 - s)
    q = v*(1 - s * f)
    t = v * (1 - s * (1 - f))
    i %= 6
    if i == 0:
        return (v, t, p)
    if i == 1:
        return (q, v, p)
    if i == 2:
        return (p, v, t)
    if i == 3:
        return (p, q, v)
    if i == 4:
        return (t, p, v)
    if i == 5:
        return (v, p, q)


def rgb_to_hsv(r, g, b):
    mx = max(r, g, b)
    mn = min(r, g, b)
    df = mx-mn
    if mx == mn:
        h = 0
    elif mx == r:
        h = (60 * ((g - b) / df) + 360) % 360
    elif mx == g:
        h = (60 * ((b - r) / df) + 120) % 360
    elif mx == b:
        h = (60 * ((r - g) / df) + 240) % 360
    if mx == 0:
        s = 0
    else:
        s = (df / mx) * 100
    v = mx * 100
    return h, s, v


def hsv_to_rgb_hex(h, s, v):
    r, g, b = hsv_to_rgb(h / 255, s / 255, v / 255)
    return f"#{int(r * 255):02x}{int(g * 255):02x}{int(b * 255):02x}"


class Colours(Modal):
    FONT = ("SegoeUI", 10)
    LABELS = ["Zone 1", "Zone 2", "Zone 3", "Zone 1", "Zone 2", "Zone 3"]

    def __init__(self, parent, colours_left, colours_right, callback):
        super().__init__(parent)
        self.title("Select colours")
        self.resizable(0, 0)

        self.colours_left = colours_left
        self.colours_right = colours_right
        self.frames = [None] * (len(colours_left) + len(colours_right))

        def add_btn(idx, col, row):
            f = tk.Frame(self, background=self.idx_to_colour(idx))
            ttk.Button(f, text=self.LABELS[idx], command=lambda *_: self._pick_colour(idx)).pack(padx=32, pady=16)
            f.grid(column=col, row=row)
            self.frames[idx] = f

        add_btn(0, 0, 0)
        add_btn(1, 0, 1)
        add_btn(2, 0, 2)
        add_btn(3, 1, 0)
        add_btn(4, 1, 1)
        add_btn(5, 1, 2)

        ttk.Button(self, text="Done", command=lambda *_: self._close()).grid(column=1, row=3, pady=16)

        self.callback = callback

    def _close(self):
        super()._close()
        self.callback()

    def idx_to_colour(self, idx):
        if idx < len(self.colours_left):
            col = self.colours_left[idx]
        else:
            col = self.colours_right[idx - len(self.colours_left)]
        return hsv_to_rgb_hex(col.h, col.s, col.v)

    def _pick_colour(self, idx):
        new = askcolor(color=self.idx_to_colour(idx))
        if not new[0]:
            return
        (r, g, b), hex_ = new
        h, s, v = rgb_to_hsv(r / 255, g / 255, b / 255)
        colour = [int((h / 360) * 255), int((s / 100) * 255), int((v / 100) * 255)]

        if idx < len(self.colours_left):
            self.colours_left[idx] = CHSV(*colour)
        else:
            self.colours_right[idx - len(self.colours_left)] = CHSV(*colour)

        self.frames[idx].configure(background=hex_)


class Keybinds(Modal):
    FONT = ("SegoeUI", 10)
    LABELS = ["BT-A", "BT-B", "BT-C", "BT-D", "FX-L", "FX-R", "Start", "EX-1", "EX-2", "EX-3"]

    def __init__(self, parent, bindings, callback):
        super().__init__(parent)
        self.title("Set keybinds")
        self.resizable(0, 0)

        self.buttons = [None] * len(bindings)

        def add_btn(idx, col, row, span=1):
            btn = ttk.Button(
                self,
                text=f"{self.LABELS[idx]} ({chr(bindings[idx])})",
                command=lambda *_: self._pick_key(idx, self.LABELS[idx])
            )
            btn.grid(column=col, row=row, columnspan=span)
            self.buttons[idx] = btn

        add_btn(6, 1, 0, 2)  # Start
        add_btn(0, 0, 1)  # BT-A
        add_btn(1, 1, 1)  # BT-B
        add_btn(2, 2, 1)  # BT-C
        add_btn(3, 3, 1)  # BT-D
        add_btn(4, 0, 2, 2)  # FX-L
        add_btn(5, 2, 2, 2)  # FX-R
        tk.Frame(self, height=20).grid(column=0, row=3, columnspan=4)
        add_btn(7, 0, 4)  # EX-1
        add_btn(8, 1, 4)  # EX-2
        add_btn(9, 3, 4)  # EX-3

        tk.Frame(self, height=40).grid(column=0, row=5, columnspan=4)
        ttk.Button(self, text="Done", command=lambda *_: self._close()).grid(column=3, row=6)

        self.bindings = list(bindings)
        self.callback = callback

    def _close(self):
        super()._close()
        self.callback(self.bindings)

    def _pick_key(self, index, label):
        Keybinder(self, label, self.bindings[index], self._cb(index))

    def _cb(self, index):
        def callback(key):
            self.bindings[index] = key
            self.buttons[index].configure(text=f"{self.LABELS[index]} ({chr(key)})")
        return callback


class GUI:
    FONT = ("SegoeUI", 10)
    FONT_B = ("SegoeUI", 10, "bold")
    FONT_L = ("SegoeUI", 8)

    def __init__(self):
        self.messages = []

        self.root = tk.Tk()
        self.root.resizable(0, 0)
        self.root.title("YuanCon (CFW) configuration tool")

        self.ndf = tk.Frame(self.root)
        tk.Label(self.ndf, text="Waiting for controller", font=self.FONT_B).grid(sticky=tk.W, row=0, column=0)
        tk.Label(self.ndf, text="Connect a YuanCon to continue", font=self.FONT).grid(sticky=tk.W, row=1, column=0)
        self.ndf.pack(expand=True, fill=tk.X, padx=40, pady=20)

        self.df = tk.Frame(self.root)
        tk.Label(self.df, text="Connected to controller", font=self.FONT_B, anchor=tk.W).pack(expand=True, fill=tk.X)

        self.led_start_combo = self._make_combo(self.df, "Start lighting mode", (
            "Rainbow",
            "Solid colour",
            "HID lighting",
            "Disabled",
        ))
        self.led_wing_upper_combo = self._make_combo(self.df, "Upper wing lighting mode", (
            "Rainbow",
            "Solid colour",
            "HID lighting",
            "Disabled",
        ))
        self.led_wing_lower_combo = self._make_combo(self.df, "Lower wing lighting mode", (
            "Rainbow",
            "Solid colour",
            "HID lighting",
            "Disabled",
        ))
        self.led_button_combo = self._make_combo(self.df, "Button lighting mode", (
            "React to button presses",
            "HID lighting",
            "Combined reactive + HID lighting",
            "Disabled",
        ))
        self.led_laser_combo = self._make_combo(self.df, "Laser lighting mode", (
            "White",
            "Solid colour",
            "Disabled",
        ))

        self.auto_hid_iv = tk.IntVar()
        self.auto_hid_cb = ttk.Checkbutton(
            self.df,
            variable=self.auto_hid_iv,
            text="Automatically switch to HID mode",
            command=lambda *_: self._set_info()
        )
        self.auto_hid_cb.pack(anchor=tk.W, pady=(10, 0))

        self.reactive_buttons_iv = tk.IntVar()
        self.reactive_buttons_cb = ttk.Checkbutton(
            self.df,
            variable=self.reactive_buttons_iv,
            text="Reactive buttons",
            command=lambda *_: self._set_info()
        )
        self.reactive_buttons_cb.pack(anchor=tk.W)

        ttk.Button(self.df, text="Edit colours", command=lambda *_: self._do_colours()).pack(anchor=tk.W)

        ttk.Separator(self.df, orient="horizontal").pack(fill="x", pady=10)

        self.con_mode_combo = self._make_combo(self.df, "Controller input mode", (
            "Keyboard + Mouse + Gamepad",
            "Keyboard + Mouse",
            "Gamepad (absolute knob position)",
            "Gamepad (knob direction)",
        ))
        self.ekb_btn = ttk.Button(self.df, text="Edit key bindings", command=lambda *_: self._do_keybinds())
        self.ekb_btn.pack(anchor=tk.W, pady=(4, 0))

        ttk.Separator(self.df, orient="horizontal").pack(fill="x", pady=10)
        ttk.Button(self.df, text="Save", command=lambda *_: self._command(b"C")).pack(anchor=tk.W)
        ttk.Button(self.df, text="Reset", command=lambda *_: self._command(b"c")).pack(anchor=tk.W)

        self._serial: serial.Serial = None
        self._con_info: ConInfo = None
        self.root.after(0, self.detect_device)

    def _make_combo(self, parent, label, options):
        tk.Label(parent, text=label, font=self.FONT, anchor=tk.W).pack(expand=True, fill=tk.X)
        combo = ttk.Combobox(parent)
        combo["values"] = options
        combo["state"] = "readonly"
        combo.bind('<<ComboboxSelected>>', lambda *_: self._set_info())
        combo.pack(expand=True, fill=tk.X)
        return combo

    def _do_colours(self):
        Colours(self.root, self._con_info.led_solid_l, self._con_info.led_solid_r, self._set_info)

    def _do_keybinds(self):
        Keybinds(self.root, self._con_info.keymap, self._keybinds_cb)

    def _keybinds_cb(self, keymap):
        self._con_info.keymap = bytes(keymap)
        self._set_info()

    def _get_info(self):
        if self._serial is None:
            return

        self._serial.write(b's')
        version = self._serial.read(1)[0]
        size = self._serial.read(1)[0]
        config = self._serial.read(size)
        # TODO: Nicer, lol.
        assert version == PERSIST_DATA_VERSION
        assert size == ctypes.sizeof(ConInfo)

        self._con_info = ConInfo.from_buffer_copy(config)

        if self._con_info.con_mode == 0:
            self.ekb_btn.configure(state="normal")
        else:
            self.ekb_btn.configure(state="disabled")

        try:
            self.led_laser_combo.current(self._con_info.led_mode.lasers)
            self.led_start_combo.current(self._con_info.led_mode.start)
            self.led_wing_upper_combo.current(self._con_info.led_mode.wing_upper)
            self.led_wing_lower_combo.current(self._con_info.led_mode.wing_lower)
            self.led_button_combo.current(self._con_info.led_mode.buttons)
            self.auto_hid_iv.set(self._con_info.auto_hid)
            self.reactive_buttons_iv.set(self._con_info.reactive_buttons)
            self.con_mode_combo.current(self._con_info.con_mode)
        except Exception:
            raise
            # TODO: More granular
            pass

    def _set_info(self):
        if self._serial is None:
            return

        self._con_info.led_mode.lasers = self.led_laser_combo.current()
        self._con_info.led_mode.start = self.led_start_combo.current()
        self._con_info.led_mode.wing_upper = self.led_wing_upper_combo.current()
        self._con_info.led_mode.wing_lower = self.led_wing_lower_combo.current()
        self._con_info.led_mode.buttons = self.led_button_combo.current()
        self._con_info.auto_hid = self.auto_hid_iv.get()
        self._con_info.reactive_buttons = self.reactive_buttons_iv.get()
        self._con_info.con_mode = self.con_mode_combo.current()

        self._serial.write(b"S")
        self._serial.write(bytes(self._con_info))
        assert self._serial.read(1) == b"S"

        # We might as well
        self._get_info()

    def _command(self, cmd):
        if self._serial is None:
            return
        self._serial.write(cmd)
        assert self._serial.read(1) == cmd

    def _reboot(self, bootloader=False):
        if self._serial is None:
            return
        if bootloader:
            self._serial.write(b'R')
        else:
            self._serial.write(b'r')

    def switch_from(self, old, new):
        old.pack_forget()
        new.pack(expand=True, fill=tk.X, padx=40, pady=20)

    def detect_device(self):
        port = find_yuan_port()

        if port is None:
            return self.root.after(100, self.detect_device)
        self._serial = serial.Serial(port, baudrate=BAUDRATE)
        self._get_info()

        self.switch_from(self.ndf, self.df)

    def main(self):
        self.root.mainloop()


if __name__ == "__main__":
    GUI().main()
