import struct
import argparse
import sys
import time
import os

from tkinter import Frame, Label, StringVar, Tk, messagebox, PhotoImage
from tkinter.filedialog import askopenfilename
from tkinter.ttk import Button, Entry, Progressbar

import serial

from util import real_path, find_port, RESET_BAUDRATE, CONFIG_BAUDRATE, SAM_BA_BAUDRATE, SAMD21_DEVICE_ID
from samba import SAMD21


class Flasher():
    def __init__(self):
        self.args = self.parse_args()

        self.root = Tk()
        self.root.resizable = False
        self.root.title("Firmware Updater")

        art = PhotoImage(file=real_path("updater.png"))
        self.art = Label(self.root, image=art)
        self.art.image = art
        self.art.grid(padx=20, pady=20, row=0)

        controls = Frame(self.root)

        self.fw_path_sv = StringVar()
        self.fw_path_sv.trace("w", lambda *_: self.fw_path_cb())
        self.fw_path = Entry(controls, textvariable=self.fw_path_sv, width=80)
        self.fw_path.grid(row=0, column=0, sticky="we", pady=2, padx=2)
        self.fw_path_btn = Button(controls, command=self.choose, text="Choose firmware", width=20)
        self.fw_path_btn.grid(row=0, column=1, sticky="we", pady=2, padx=2)

        self.pb = Progressbar(controls)
        self.pb.grid(row=1, column=0, sticky="we", pady=2, padx=2)
        self.flash_btn = Button(controls, command=self.flash, text="Flash", state="disabled")
        self.flash_btn.grid(row=1, column=1, sticky="we", pady=2, padx=2)

        self._status = Label(controls)
        self._status.grid(row=2, column=0, columnspan=2, pady=(2, 0), padx=2)

        controls.grid(pady=(0, 15), row=1)

        self.com = None

        if self.args.firmware:
            self.fw_path.insert("end", self.args.firmware)
            if self.args.auto:
                self.root.after(0, self.flash)

    def status(self, text):
        print("I:" + text, file=sys.stderr, flush=True)
        self._status.configure(text=text)
        self.root.update()

    def parse_args(self):
        parser = argparse.ArgumentParser(description="YuanCon firmware updater")
        parser.add_argument("firmware", type=str, nargs="?", help="Path to firmware '.bin' file")

        parser.add_argument(
            "-a", "--auto", action="store_true",
            help="Automatically begin uploading (must be used with a firmware path)")
        parser.add_argument(
            "-c", "--close_after", action="store_true",
            help="Close after succesfully uploading")
        # parser.add_argument(
        #     "-n", "--no_gui", action="store_true",
        #     help="Upload without showing the GUI")

        return parser.parse_args()

    def _com_check(self):
        prog = find_port(True)
        if prog:
            self._got_com(prog, True)
            return
        con = find_port(False)
        if con:
            self._got_com(con, False)
            return

        self.root.after(100, self._com_check)

    def _update_done(self):
        self._status.configure(text="")
        self.fw_path.configure(state="normal")
        self.fw_path_btn.configure(state="normal")
        self.fw_path_cb()
        self.pb.configure(value=0)

        if self.args.close_after:
            self.root.destroy()

    def is_com_yuan_cfw(self, com):
        com.baudrate = SAM_BA_BAUDRATE
        com.write(b"V")
        time.sleep(0.5)
        if not com.in_waiting:
            com.baudrate = CONFIG_BAUDRATE
            return False

        version = b""
        while not version.endswith(b"\r\n"):
            version += com.read(1)
        self.status(f"Version: {version.decode().strip()}")
        self.root.update()

        com.write(b"w41002018,4#")  # Get device id
        device_id = struct.unpack("<I", com.read(4))[0]
        com.baudrate = CONFIG_BAUDRATE

        return device_id == SAMD21_DEVICE_ID

    def try_get_settings(self, name):
        com = serial.Serial(name, baudrate=CONFIG_BAUDRATE)
        if not self.is_com_yuan_cfw(com):
            return None

        com.write(b"s")
        version = com.read(1)[0]
        size = com.read(1)[0]
        config = com.read(size)
        return version, config

    def try_write_settings(self, name, settings):
        version, data = settings

        com = serial.Serial(name, baudrate=CONFIG_BAUDRATE)
        if not self.is_com_yuan_cfw(com):
            return None

        com.write(b"s")
        version_check = com.read(1)[0]
        com.read(com.read(1)[0])  # We don't care for the default values

        if version_check != version:
            messagebox.showwarning("Update", "Version missmatch.\nSettings have not been restored.")
            return

        com.write(b"c" + data + b"C")

    def _got_com(self, name, is_programming):
        # If this COM was just plugged in, it needs a moment to start existing
        time.sleep(0.2)

        if is_programming:
            settings = None
        else:
            settings = self.try_get_settings(name)

        if not is_programming:
            self.status("Controller found. Switching to programming mode")
            serial.Serial(name, RESET_BAUDRATE).close()

            start = time.time()
            while time.time() - start < 3:
                name = find_port(True)
                if name:
                    break
                self.root.update()
                time.sleep(0.1)
            else:
                self.root.update()
                messagebox.showerror("Update", "Controller didn't respond. Please try again.")
                self._update_done()
                return

            # Give the COM a moment to actually wake up
            for _ in range(5):
                self.root.update()
                time.sleep(0.1)

        self.status(f"Using {name} for update")

        # TODO: Basic sanity checks on the bin file
        firmware = open(self.fw_path.get(), "rb").read()

        with SAMD21(name) as samd:
            self.status("Bootloader: " + samd.get_version())
            self.status("Erasing chip")
            samd.chip_erase()
            self.status("Writing firmware")
            start = time.perf_counter()

            def callback(current, total):
                self.pb.configure(value=(current / total) * 100)
                delta = time.perf_counter() - start
                self.status(f"Writing firmware ({total / delta / 1024:.3f} KiB/s)")
                self.root.update()

            try:
                samd.write_program(firmware, callback=callback)
            except ValueError:
                self.status("Checksum validation failed")
                messagebox.showerror("Update", "Checksum validation failed.\nPlease try again.")
                return

            self.status("Firmware written, restarting")
            samd.reset()

        if settings:
            self.status("Restoring settings")
            name = None
            for _ in range(20):
                self.root.update()
                time.sleep(0.1)
                name = find_port()
                if name:
                    break

            if name:
                time.sleep(0.2)
                self.try_write_settings(name, settings)

        if not (self.args.auto and self.args.close_after):
            messagebox.showinfo("Update", "Update completed")
        self._update_done()

    def fw_path_cb(self):
        if os.path.isfile(self.fw_path.get()):
            self.flash_btn.configure(state="normal")
        else:
            self.flash_btn.configure(state="disabled")

    def choose(self):
        filename = askopenfilename(title="Choose firmware", filetypes=(
            ("Firmware files", "*.bin"),
            ("All files", "*.*"),
        ))
        if not filename:
            return

        self.fw_path.delete(0, len(self.fw_path.get()))
        self.fw_path.insert("end", filename)

    def flash(self):
        self.fw_path.configure(state="disabled")
        self.fw_path_btn.configure(state="disabled")
        self.flash_btn.configure(state="disabled")
        self.status("Waiting for controller...")
        self.root.after(0, self._com_check)


if __name__ == "__main__":
    Flasher().root.mainloop()
