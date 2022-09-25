import ctypes

import tkinter as tk
from tkinter import messagebox
import tkinter.ttk as ttk
from tkinter.colorchooser import askcolor

import serial

from persist import ConInfo, CHSV
from util import (
    CONFIG_BAUDRATE, SAM_BA_BAUDRATE,
    get_com_serial, find_port,
    KEYBOARD_MODE, GAMEPAD_MODE, MOUSE_MODE, GAMEPAD_MODE_POS, GAMEPAD_MODE_DIR
)


VENDOR_NAME = "YuanCon"
PRODUCT_NAME = "YuanCon"

PERSIST_DATA_VERSION = 2

FONT = ("SegoeUI", 10)
FONT_B = ("SegoeUI", 10, "bold")

BUTTON_NAMES = ["BT-A", "BT-B", "BT-C", "BT-D", "FX-L", "FX-R", "Start", "EX-1", "EX-2", "EX-3"]
NUM_KEYS = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "+", "-", "/", "*", "ENTER", "."]


class Modal(tk.Toplevel):
    def __init__(self, parent, hidden=False):
        super().__init__(parent, bg="#fff")
        if not hidden:
            self.wait_visibility()
            self.grab_set()
            self.lift()
        self.bind("<1>", self._capture_click)
        self.protocol('WM_DELETE_WINDOW', self._close)

    def show(self):
        self.deiconify()
        self.wait_visibility()
        self.grab_set()
        self.lift()

    def hide(self):
        self.grab_release()
        self.withdraw()

    def _capture_click(self, event):
        if event.widget == self:
            if event.x < 0 or event.x > self.winfo_width() or event.y < 0 or event.y > self.winfo_height():
                self.bell()
                self.focus()

    def _close(self):
        self.grab_release()
        self.destroy()


class Keybinder(Modal):
    def __init__(self, parent, escape_valid=False):
        super().__init__(parent, hidden=True)
        self.geometry("200x100")
        self.resizable(0, 0)
        self.grab_release()
        self.withdraw()

        self.title("Bind key")

        kb_frame = Frame(self)
        kb_frame.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

        self.bind("<KeyPress>", self._bind_key)
        self.focus_set()

        self.for_lab = label(kb_frame, "", font=FONT_B)
        self.for_lab.pack()
        self.bind_lab = label(kb_frame, "", font=FONT_B)
        self.bind_lab.pack()

        self.escape_valid = escape_valid
        self.callback = None

    def do_bind(self, name, key, callback):
        self.for_lab.configure(text=f"Press the key for {name}:")
        key_str = chr(key) if key < 0x80 else f"NUM {NUM_KEYS[key - 0x80]}"
        if not key_str.isprintable():
            key_str = ""
        self.bind_lab.configure(text=f"<{key_str}>")
        self.callback = callback
        self.show()

    NUMPAD_MAPPING = {
        # 0 - 9
        0x60: 0x80,
        0x61: 0x81,
        0x62: 0x82,
        0x63: 0x83,
        0x64: 0x84,
        0x65: 0x85,
        0x66: 0x86,
        0x67: 0x87,
        0x68: 0x88,
        0x69: 0x89,
        # +, -, *
        0x6b: 0x8a,
        0x6d: 0x8b,
        0x6a: 0x8c,
        # .
        0x6e: 0x8f,
    }
    NUMPAD_MOD_MAPPING = {
        # /, ENTER
        0x6f: 0x8d,
        0x0d: 0x8e,
    }
    NUMPAD_ALL_MAPPING = {**NUMPAD_MAPPING, **NUMPAD_MOD_MAPPING}

    def _bind_key(self, key):
        # Escape
        if key.keycode == 27:
            self.hide()
            if self.escape_valid:
                self.callback(0)
            return

        if key.char:
            # Numpad keys
            if key.keycode in self.NUMPAD_MAPPING or (key.state & 0x40000 and key.keycode in self.NUMPAD_MOD_MAPPING):
                sym = self.NUMPAD_ALL_MAPPING[key.keycode]
                self.callback(sym)
                self.bind_lab.configure(text=f"<NUM {NUM_KEYS[sym - 0x80]}>")
            else:
                self.bind_lab.configure(text=f"<{key.char}>")
                self.callback(ord(key.char))
            self.hide()


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

    def __init__(self, parent, colours, callback):
        super().__init__(parent)
        self.configure(bg="#fff")
        self.title("Select colours")
        self.resizable(0, 0)

        self.colours = colours
        self.frames = [None] * len(colours)

        def add_btn(idx, col, row):
            f = tk.Frame(self, bg=self.idx_to_colour(idx))
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
        col = self.colours[idx]
        return hsv_to_rgb_hex(col.h, col.s, col.v)

    def _pick_colour(self, idx):
        new = askcolor(color=self.idx_to_colour(idx))
        if not new[0]:
            return
        (r, g, b), hex_ = new
        h, s, v = rgb_to_hsv(r / 255, g / 255, b / 255)
        colour = [int((h / 360) * 255), int((s / 100) * 255), int((v / 100) * 255)]

        self.colours[idx] = CHSV(*colour)
        self.frames[idx].configure(background=hex_)


def digits_only(action, index, value_if_allowed, *_):
    if not value_if_allowed:
        return True
    try:
        float(value_if_allowed)
        return True
    except ValueError:
        pass

    if value_if_allowed == "-":
        return True

    return False


def max32(action, index, value_if_allowed, *_):
    return len(value_if_allowed) <= 32


class Frame(tk.Frame):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.configure(bg="#fff")


class ConInfoFrame(Frame):
    def __init__(self, parent):
        super().__init__(parent)

        label(self, "Controller Name", font=FONT_B).grid(row=0, column=0, sticky=tk.W, padx=4)
        label(self, "Firmware Version", font=FONT_B).grid(row=0, column=1, sticky=tk.W, padx=4)
        label(self, "Serial Number", font=FONT_B).grid(row=0, column=2, sticky=tk.W, padx=4)

        self.name = label(self, "")
        self.name.grid(row=1, column=0, sticky=tk.W, padx=4)
        self.version = label(self, "")
        self.version.grid(row=1, column=1, sticky=tk.W, padx=4)
        self.serial = label(self, "")
        self.serial.grid(row=1, column=2, sticky=tk.W, padx=4)

    def populate(self, com, con_info):
        self.name.configure(text="YuanCon")
        com.baudrate = SAM_BA_BAUDRATE
        com.write(b"V")
        version = b""
        while not version.endswith(b"\r\n"):
            version += com.read(1)
        com.baudrate = CONFIG_BAUDRATE
        self.version.configure(text=version.decode("latin-1").strip())

        self.serial.configure(text=get_com_serial(com))


def label(parent, text, **kwargs):
    return ttk.Label(parent, text=text, **kwargs)


def grid(widget, col=0, span=3, incr=None, **kwargs):
    if "sticky" not in kwargs:
        kwargs["sticky"] = tk.W
    if "pady" not in kwargs:
        kwargs["pady"] = 2

    widget.grid(row=widget.master._grid_y, column=col, columnspan=span, **kwargs)
    if incr or (incr is None and col + span == 3):
        widget.master._grid_y += 1
    return widget


def make_combo(parent, label_, options, pair=False):
    label(parent, label_, anchor=tk.W).grid(row=parent._grid_y, column=0, sticky=tk.W)

    combo1 = ttk.Combobox(parent)
    combo1["values"] = options
    combo1["state"] = "readonly"
    combo1.bind('<<ComboboxSelected>>', lambda *_: parent.update())
    grid(combo1, col=1, span=1 if pair else 2, pady=2, sticky="we")
    if not pair:
        return combo1

    combo2 = ttk.Combobox(parent)
    combo2["values"] = options
    combo2["state"] = "readonly"
    combo2.bind('<<ComboboxSelected>>', lambda *_: parent.update())
    grid(combo2, col=2, span=1, padx=(5, 0), pady=2, sticky="we")
    return combo1, combo2


class NoDeviceFound(Frame):
    def __init__(self, parent):
        super().__init__(parent, padx=40, pady=20)

        label(self, "Waiting for controller", font=FONT_B).grid(sticky=tk.W, row=0, column=0)
        label(self, "Connect a YuanCon to continue", font=FONT).grid(sticky=tk.W, row=1, column=0)


class LightingSettings(Frame):
    def __init__(self, parent, update):
        super().__init__(parent, padx=8, pady=8)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.columnconfigure(2, weight=1)
        self.update = update
        self.zone_colours = None

        vcmd = (self.register(digits_only), '%d', '%i', '%P', '%s', '%S', '%v', '%V', '%W')
        self._grid_y = 0

        self.led_start_combo = make_combo(self, "Start lighting mode", (
            "Rainbow",
            "Solid colour",
            "HID lighting",
            "Disabled",
        ), True)
        self.led_wing_upper_combo = make_combo(self, "Upper wing lighting mode", (
            "Rainbow",
            "Solid colour",
            "HID lighting",
            "Disabled",
        ), True)
        self.led_wing_lower_combo = make_combo(self, "Lower wing lighting mode", (
            "Rainbow",
            "Solid colour",
            "HID lighting",
            "Disabled",
        ), True)
        self.led_button_combo = make_combo(self, "Button lighting mode", (
            "React to button presses",
            "HID lighting",
            "Combined reactive + HID lighting",
            "Disabled",
        ))
        self.led_laser_combo = make_combo(self, "Laser lighting mode", (
            "White",
            "Solid colour",
            "Disabled",
        ))

        grid(label(self, "LED saturation (applies across all zones in rainbow mode, 0-255)"), span=1)
        self.leds_sat = ttk.Entry(self, validate="key", validatecommand=vcmd)
        grid(self.leds_sat, col=1, span=2, sticky=tk.W + tk.E)
        self.leds_sat.bind("<FocusOut>", lambda *_: self.update())

        self.auto_hid_iv = tk.IntVar()
        self.auto_hid_cb = grid(ttk.Checkbutton(
            self,
            variable=self.auto_hid_iv,
            text="Automatically switch to HID mode",
            command=lambda *_: self.update()
        ), pady=(10, 0))

        self.reactive_buttons_iv = tk.IntVar()
        self.reactive_buttons_cb = grid(ttk.Checkbutton(
            self,
            variable=self.reactive_buttons_iv,
            text="Reactive buttons",
            command=lambda *_: self.update()
        ))

        grid(ttk.Button(self, text="Edit colours", command=lambda *_: self._do_colours()))
        grid(label(self, "Dim LEDs after (minutes, -1 to disable)"), span=1)
        self.leds_dim = ttk.Entry(self, validate="key", validatecommand=vcmd)
        grid(self.leds_dim, col=1, span=2, sticky=tk.W + tk.E)
        self.leds_dim.bind("<FocusOut>", lambda *_: self.update())

        grid(label(self, "Turn off LEDs after (minutes, -1 to disable)"), span=1)
        self.leds_off = ttk.Entry(self, validate="key", validatecommand=vcmd)
        grid(self.leds_off, col=1, span=2, sticky=tk.W + tk.E)
        self.leds_off.bind("<FocusOut>", lambda *_: self.update())

    def _do_colours(self):
        Colours(self, self.zone_colours, self.update)

    def populate(self, con_info):
        self.zone_colours = con_info.zone_colours

        self.led_start_combo[0].current(con_info.zone_modes[0])
        self.led_start_combo[1].current(con_info.zone_modes[3])

        self.led_wing_upper_combo[0].current(con_info.zone_modes[1])
        self.led_wing_upper_combo[1].current(con_info.zone_modes[4])

        self.led_wing_lower_combo[0].current(con_info.zone_modes[2])
        self.led_wing_lower_combo[1].current(con_info.zone_modes[5])

        self.led_laser_combo.current(con_info.led_mode.lasers)
        self.led_button_combo.current(con_info.button_lights)

        self.auto_hid_iv.set(con_info.auto_hid)
        self.reactive_buttons_iv.set(con_info.reactive_buttons)

        self.leds_sat.delete(0, len(self.leds_sat.get()))
        self.leds_sat.insert("end", str(con_info.saturation))

        self.leds_dim.delete(0, len(self.leds_dim.get()))
        if con_info.led_timeout == 0xffff:
            self.leds_dim.insert("end", "-1")
        else:
            self.leds_dim.insert("end", str(round(con_info.led_dim / 60, 2)))

        self.leds_off.delete(0, len(self.leds_off.get()))
        if con_info.led_timeout == 0xffff:
            self.leds_off.insert("end", "-1")
        else:
            self.leds_off.insert("end", str(round(con_info.led_timeout / 60, 2)))

    def fill(self, con_info):
        con_info.zone_modes = (
            self.led_start_combo[0].current(),
            self.led_wing_upper_combo[0].current(),
            self.led_wing_lower_combo[0].current(),
            self.led_start_combo[1].current(),
            self.led_wing_upper_combo[1].current(),
            self.led_wing_lower_combo[1].current(),
        )
        con_info.led_mode.lasers = self.led_laser_combo.current()
        con_info.led_mode.button_lights = self.led_button_combo.current()

        con_info.auto_hid = self.auto_hid_iv.get()
        con_info.reactive_buttons = self.reactive_buttons_iv.get()

        try:
            con_info.saturation = max(0, min(255, int(self.leds_sat.get())))
        except ValueError:
            pass

        if self.leds_dim.get() == "-1":
            con_info.led_dim = 0xffff
        else:
            try:
                val = float(self.leds_dim.get())
                con_info.led_dim = max(0, min(0xfffe, int(val * 60)))
            except ValueError:
                pass

        if self.leds_off.get() == "-1":
            con_info.led_timeout = 0xffff
        else:
            try:
                val = float(self.leds_off.get())
                con_info.led_timeout = max(0, min(0xfffe, int(val * 60)))
            except ValueError:
                pass


class KeyboardSettings(Frame):
    def __init__(self, parent, update):
        super().__init__(parent, padx=8, pady=8)
        self.update = update
        self.keymap = None
        self._grid_y = 0

        self.enabled_iv = tk.IntVar()
        self.enabled = grid(ttk.Checkbutton(
            self,
            variable=self.enabled_iv,
            text="Keyboard Enabled",
            command=lambda *_: self.update()
        ))

        self.binding_frame = grid(Frame(self))
        self.buttons = []

        self.keybinder = None

    def _cb(self, index):
        def callback(key):
            self.keymap[index] = key

            key_str = chr(key) if key < 0x80 else f"NUM {NUM_KEYS[key - 0x80]}"
            self.buttons[index].configure(text=f"{BUTTON_NAMES[index]} ({key_str})")

            self.update()
        return callback

    def _pick_key(self, index, label):
        if self.keybinder is None:
            self.keybinder = Keybinder(self)
        self.keybinder.do_bind(label, self.keymap[index], self._cb(index))

    def create_buttons(self):
        self.buttons = [None] * len(self.keymap)

        def add_btn(idx, col, row, span=1):
            key = self.keymap[idx]
            key_str = chr(key) if key < 0x80 else f"NUM {NUM_KEYS[key - 0x80]}"

            btn = ttk.Button(
                self.binding_frame,
                text=f"{BUTTON_NAMES[idx]} ({key_str})",
                command=lambda *_: self._pick_key(idx, BUTTON_NAMES[idx])
            )
            btn.grid(column=col, row=row, columnspan=span, padx=2, pady=2)
            self.buttons[idx] = btn

        add_btn(6, 1, 0, 2)  # Start
        add_btn(0, 0, 1)  # BT-A
        add_btn(1, 1, 1)  # BT-B
        add_btn(2, 2, 1)  # BT-C
        add_btn(3, 3, 1)  # BT-D
        add_btn(4, 0, 2, 2)  # FX-L
        add_btn(5, 2, 2, 2)  # FX-R
        Frame(self.binding_frame, height=20).grid(column=0, row=3, columnspan=4)
        add_btn(7, 0, 4)  # EX-1
        add_btn(8, 1, 4)  # EX-2
        add_btn(9, 3, 4)  # EX-3

    def _enabled_changed(self):
        if self.enabled_iv.get():
            for i in self.buttons:
                i.configure(state="normal")
        else:
            for i in self.buttons:
                i.configure(state="disabled")

    def populate(self, con_info):
        self.keymap = list(con_info.keymap)
        self.enabled_iv.set(bool(con_info.con_mode & KEYBOARD_MODE))
        if not self.buttons:
            self.create_buttons()
        self._enabled_changed()

    def fill(self, con_info):
        con_info.keymap = bytes(self.keymap)

        if self.enabled_iv.get():
            con_info.con_mode |= KEYBOARD_MODE
        else:
            con_info.con_mode &= ~KEYBOARD_MODE


class GamepadSettings(Frame):
    def __init__(self, parent, update):
        super().__init__(parent, padx=8, pady=8)
        self.update = update
        self.keymap = None
        self._grid_y = 0

        self.enabled_iv = tk.IntVar()
        self.enabled = grid(ttk.Checkbutton(
            self,
            variable=self.enabled_iv,
            text="Gamepad Enabled",
            command=lambda *_: self.update()
        ))

        self.absolute_iv = tk.IntVar()
        self.absolute = grid(ttk.Checkbutton(
            self,
            variable=self.absolute_iv,
            text="Report absolute knob position",
            command=lambda *_: self.update()
        ))

        self.inputs = [self.absolute]

        self.binding_frame = grid(Frame(self))
        self.buttons = []

    def create_buttons(self):
        self.buttons = [None] * len(self.keymap)

        def add_btn(idx, col, row, span=1):
            combo = ttk.Combobox(
                self.binding_frame,
                values=[str(i) for i in range(10)],
                width=10,
            )
            combo.set(f"{BUTTON_NAMES[idx]} ({self.keymap[idx] + 1})")

            def cb(*_):
                self.keymap[idx] = combo.current()
                combo.set(f"{BUTTON_NAMES[idx]} ({combo.current() + 1})")
                combo.selection_clear()
                self.focus()
                self.update()

            combo.bind('<<ComboboxSelected>>', cb)
            combo.grid(column=col, row=row, columnspan=span, padx=2, pady=2)
            self.buttons[idx] = combo

        add_btn(6, 1, 0, 2)  # Start
        add_btn(0, 0, 1)  # BT-A
        add_btn(1, 1, 1)  # BT-B
        add_btn(2, 2, 1)  # BT-C
        add_btn(3, 3, 1)  # BT-D
        add_btn(4, 0, 2, 2)  # FX-L
        add_btn(5, 2, 2, 2)  # FX-R
        Frame(self.binding_frame, height=20).grid(column=0, row=3, columnspan=4)
        add_btn(7, 0, 4)  # EX-1
        add_btn(8, 1, 4)  # EX-2
        add_btn(9, 3, 4)  # EX-3

    def _enabled_changed(self):
        if self.enabled_iv.get():
            for i in self.inputs:
                i.configure(state="normal")
            for i in self.buttons:
                i.configure(state="normal")
        else:
            for i in self.inputs:
                i.configure(state="disabled")
            for i in self.buttons:
                i.configure(state="disabled")

    def populate(self, con_info):
        self.keymap = con_info.gamepad_map
        self.enabled_iv.set(bool(con_info.con_mode & GAMEPAD_MODE))
        self.absolute_iv.set(bool(con_info.con_mode & GAMEPAD_MODE_POS))
        if not self.buttons:
            self.create_buttons()
        self._enabled_changed()

    def fill(self, con_info):
        con_info.gamepad_map = (ctypes.c_uint8 * len(self.keymap))(*self.keymap)

        con_info.con_mode &= ~GAMEPAD_MODE
        if self.enabled_iv.get():
            if self.absolute_iv.get():
                con_info.con_mode |= GAMEPAD_MODE_POS
            else:
                con_info.con_mode |= GAMEPAD_MODE_DIR


class MouseSettings(Frame):
    def __init__(self, parent, update):
        super().__init__(parent, padx=8, pady=8)
        self.update = update
        self._grid_y = 0

        self.enabled_iv = tk.IntVar()
        self.enabled = grid(ttk.Checkbutton(
            self,
            variable=self.enabled_iv,
            text="Mouse Enabled",
            command=lambda *_: self.update()
        ))

        self.inputs = []

    def _enabled_changed(self):
        if self.enabled_iv.get():
            for i in self.inputs:
                i.configure(state="normal")
        else:
            for i in self.inputs:
                i.configure(state="disabled")

    def populate(self, con_info):
        self.keymap = con_info.keymap
        self.enabled_iv.set(bool(con_info.con_mode & MOUSE_MODE))
        self._enabled_changed()

    def fill(self, con_info):
        if self.enabled_iv.get():
            con_info.con_mode |= MOUSE_MODE
        else:
            con_info.con_mode &= ~MOUSE_MODE


class MacroSettings(Frame):
    OPTIONS = [
        "Custom key",
        "Long macro 1",
        "Long macro 2",
        "Short macro 1",
        "Short macro 2",
        "Short macro 3",
        "Short macro 4",
    ]

    def __init__(self, parent, update):
        super().__init__(parent, padx=8, pady=8)
        self.update = update
        self._grid_y = 0

        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.columnconfigure(2, weight=1)

        self.large_macros = []

        vcmd = (self.register(max32), '%d', '%i', '%P', '%s', '%S', '%v', '%V', '%W')

        grid(label(self, "Long macro 1 (max length 32):"), span=1)
        self.lm_1 = grid(ttk.Entry(self, validate="key", validatecommand=vcmd), col=1, span=2, sticky="we")
        self.lm_1.bind("<FocusOut>", lambda *_: self.update())
        grid(label(self, "Long macro 2 (max length 32):"), span=1)
        self.lm_2 = grid(ttk.Entry(self, validate="key", validatecommand=vcmd), col=1, span=2, sticky="we")
        self.lm_2.bind("<FocusOut>", lambda *_: self.update())

        self.sm_buttons = []
        self.sm_keymap = []
        self.sm_frames = []

        for i in range(4):
            grid(label(self, f"Short macro {i + 1}:"))
            sm_frame = grid(Frame(self))
            self.sm_frames.append(sm_frame)

        grid(ttk.Separator(self), pady=8, sticky="we")

        self.macro_layer = []
        self.binding_frame = grid(Frame(self))
        self.combos = []

        self.keybinder = None

    def create_sm_buttons(self):
        for n, sm_frame in enumerate(self.sm_frames):
            buttons = []
            for i in range(10):
                sm_frame.columnconfigure(i, weight=1)

                buttons.append(ttk.Button(
                    sm_frame,
                    command=(lambda n, i: (lambda *_: self._pick_key(n, i)))(n, i)
                ))
                buttons[-1].grid(row=0, column=i, sticky="we")

            self.sm_buttons.append(buttons)
        self.update_sm_button_state()

    def get_option(self, idx):
        if self.macro_layer[idx] < 0xc0:
            return 0
        return self.macro_layer[idx] - 0xbf

    def create_binding_combos(self):
        self.combos = [None] * len(self.macro_layer)

        def add_btn(idx, col, row, span=1):
            combo = ttk.Combobox(
                self.binding_frame,
                values=self.OPTIONS,
                width=20,
            )
            combo.set(f"{BUTTON_NAMES[idx]} ({self.OPTIONS[self.get_option(idx)]})")

            def cb(*_):
                self.macro_layer[idx] = combo.current()
                if self.macro_layer[idx] != 0:
                    self.macro_layer[idx] += 0xbf

                combo.set(f"{BUTTON_NAMES[idx]} ({self.OPTIONS[self.get_option(idx)]})")
                combo.selection_clear()
                self.update_binding_button_state()
                self.focus()
                self.update()

            combo.bind('<<ComboboxSelected>>', cb)
            combo.grid(column=col, row=row * 2, columnspan=span, padx=2, pady=(2, 0))

            button = ttk.Button(
                self.binding_frame,
                command=lambda *_: self._pick_key(5, idx),
                width=19
            )
            button.grid(column=col, row=row * 2 + 1, columnspan=span, padx=2, pady=(0, 2))

            self.combos[idx] = (combo, button)

        add_btn(6, 1, 0, 2)  # Start
        add_btn(0, 0, 1)  # BT-A
        add_btn(1, 1, 1)  # BT-B
        add_btn(2, 2, 1)  # BT-C
        add_btn(3, 3, 1)  # BT-D
        add_btn(4, 0, 2, 2)  # FX-L
        add_btn(5, 2, 2, 2)  # FX-R
        Frame(self.binding_frame, height=20).grid(column=0, row=6, columnspan=4)
        add_btn(7, 0, 4)  # EX-1
        add_btn(8, 1, 4)  # EX-2
        add_btn(9, 3, 4)  # EX-3

        self.update_binding_button_state()

    def update_binding_button_state(self):
        for key, (_, button) in zip(self.macro_layer, self.combos):
            if key < 0xc0:
                key_str = f"{chr(key)}" if key < 0x80 else f"NUM {NUM_KEYS[key - 0x80]}"
                if key == 0:
                    key_str = "Choose key"

                button.configure(state="enabled", text=key_str)
            else:
                button.configure(state="disabled", text="Choose key")

    def update_sm_button_state(self):
        for macro in range(4):
            seen_end = False
            for i, j in zip(self.sm_buttons[macro], self.sm_keymap[macro]):
                key_str = chr(j) if j < 0x80 else f"NUM {NUM_KEYS[j - 0x80]}"
                if j == 0:
                    key_str = ""

                if seen_end:
                    i.configure(state="disabled", text="")
                else:
                    i.configure(state="normal", text=key_str)

                if j == 0:
                    seen_end = True
                    continue

    def _cb(self, macro, index):
        def callback(key):
            if macro == 5:
                self.macro_layer[index] = key
                self.update_binding_button_state()
            else:
                self.sm_keymap[macro][index] = key
                self.update_sm_button_state()
            self.update()
        return callback

    def _pick_key(self, macro, index):
        if self.keybinder is None:
            self.keybinder = Keybinder(self, escape_valid=True)
        if macro == 5:
            self.keybinder.do_bind(BUTTON_NAMES[index], self.macro_layer[index], self._cb(macro, index))
        else:
            self.keybinder.do_bind("label", self.sm_keymap[macro][index], self._cb(macro, index))

    def populate(self, con_info):
        self.sm_keymap = [list(i.keys) for i in con_info.short_macros]
        self.macro_layer = list(con_info.macro_layer)

        self.lm_1.delete(0, len(self.lm_1.get()))
        self.lm_1.insert("end", con_info.large_macros[0].keys.decode("latin-1"))
        self.lm_2.delete(0, len(self.lm_2.get()))
        self.lm_2.insert("end", con_info.large_macros[1].keys.decode("latin-1"))

        if not self.sm_buttons:
            self.create_sm_buttons()
        if not self.combos:
            self.create_binding_combos()

    def fill(self, con_info):
        # TODO: Configurable delay

        for i, j in zip(con_info.short_macros, self.sm_keymap):
            i.delay = 20
            i.keys = (ctypes.c_uint8 * 10)(*j)

        con_info.large_macros[0].delay = 20
        con_info.large_macros[0].keys = self.lm_1.get().encode("latin-1")
        con_info.large_macros[1].delay = 20
        con_info.large_macros[1].keys = self.lm_2.get().encode("latin-1")

        con_info.macro_layer = (ctypes.c_uint8 * 10)(*self.macro_layer)


class GUI:
    def __init__(self):
        self.messages = []

        self._initial_config = None

        self.root = tk.Tk()
        self.root.configure(bg="#fff")
        self.root.resizable(0, 0)
        self.root.title("YuanCon configuration tool")

        style = ttk.Style()
        style.configure("TCheckbutton", background="#fff", font=FONT)
        style.configure("TNotebook", background="#fff", font=FONT)
        style.configure("TLabel", background="#fff", font=FONT)
        style.configure("TButton", font=FONT)
        style.configure("TCombobox", font=FONT)
        style.configure("TLabel", font=FONT)

        self.ndf = NoDeviceFound(self.root)
        self.ndf.pack(expand=True, fill=tk.X)

        self.df = Frame(self.root, bg="#fff")
        self.df._grid_y = 0

        self.coninfo_frame = ConInfoFrame(self.df)
        grid(self.coninfo_frame, pady=(0, 10), sticky=tk.W + tk.E)

        notebook = ttk.Notebook(self.df)
        grid(notebook, sticky=tk.W + tk.E)#.pack(fill="both", expand=True)

        self.lighting_tab = LightingSettings(notebook, self._set_info)
        notebook.add(self.lighting_tab, text="Lighting")

        self.keyboard_tab = KeyboardSettings(notebook, self._set_info)
        notebook.add(self.keyboard_tab, text="Keyboard")

        self.gamepad_tab = GamepadSettings(notebook, self._set_info)
        notebook.add(self.gamepad_tab, text="Gamepad")

        self.mouse_tab = MouseSettings(notebook, self._set_info)
        notebook.add(self.mouse_tab, text="Mouse")

        self.macro_tab = MacroSettings(notebook, self._set_info)
        notebook.add(self.macro_tab, text="Macros")

        grid(ttk.Button(self.df, text="Save", command=self.save), col=0, span=1)
        grid(ttk.Button(self.df, text="Reset", command=self.reset), col=2, span=1, sticky=tk.E)

        self._serial: serial.Serial = None
        self._con_info: ConInfo = None
        self.root.after(0, self.detect_device)

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def on_close(self):
        self.fill()
        if self._initial_config is not None and bytes(self._con_info) != self._initial_config:
            resp = messagebox.askyesnocancel("Unsaved changes", "There are unsaved changes!\nWould you like to save them?")
            if resp is None:
                return

            if resp:
                self.save()
            try:
                self.reset()
            except Exception:
                pass
            self.root.destroy()
        else:
            try:
                self.reset()
            except Exception:
                pass
            self.root.destroy()

    def save(self, *_):
        if self._serial is None:
            return
        self._command(b"C")
        self._initial_config = None
        self._get_info()

    def reset(self, *_):
        if self._serial is None:
            return
        if self._initial_config is None:
            return

        self._serial.write(b"c")
        self._serial.write(self._initial_config)
        assert self._serial.read(1) == b"c"
        self._get_info()

    def fill(self):
        self.lighting_tab.fill(self._con_info)
        self.keyboard_tab.fill(self._con_info)
        self.gamepad_tab.fill(self._con_info)
        self.mouse_tab.fill(self._con_info)
        self.macro_tab.fill(self._con_info)

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

        if self._initial_config is None:
            self._initial_config = config

        self._con_info = ConInfo.from_buffer_copy(config)

        try:
            self.lighting_tab.populate(self._con_info)
            self.keyboard_tab.populate(self._con_info)
            self.gamepad_tab.populate(self._con_info)
            self.mouse_tab.populate(self._con_info)
            self.macro_tab.populate(self._con_info)
        except Exception:
            raise
            # TODO: More granular
            pass

        self.coninfo_frame.populate(self._serial, self._con_info)

    def _set_info(self):
        if self._serial is None:
            return

        self.fill()

        self._serial.write(b"c")
        self._serial.write(bytes(self._con_info))
        assert self._serial.read(1) == b"c"

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
        port = find_port()

        if port is None:
            return self.root.after(100, self.detect_device)

        try:
            self._serial = serial.Serial(port, baudrate=CONFIG_BAUDRATE)
        except serial.serialutil.SerialException:
            return self.root.after(100, self.detect_device)

        self._get_info()

        self.switch_from(self.ndf, self.df)

    def main(self):
        self.root.mainloop()


if __name__ == "__main__":
    GUI().main()
