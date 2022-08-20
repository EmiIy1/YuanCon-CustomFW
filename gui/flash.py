import subprocess
import argparse
import time
import os
import re

from tkinter import Frame, Label, StringVar, Tk, messagebox, PhotoImage
from tkinter.filedialog import askopenfilename
from tkinter.ttk import Button, Entry, Progressbar

import serial

from util import real_path, find_port, build_update_command


BOSSAC_PROGRESS_RE = re.compile(r"\[=* *\] \d+% \((\d+)/(\d+) pages\)")


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

        self.status = Label(controls)
        self.status.grid(row=2, column=0, columnspan=2, pady=(2, 0), padx=2)

        controls.grid(pady=(0, 15), row=1)

        self.com = None

        print([self.args.firmware])

        if self.args.firmware:
            self.fw_path.insert("end", self.args.firmware)
            if self.args.auto:
                self.root.after(0, self.flash)

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
        self.status.configure(text="")
        self.fw_path.configure(state="normal")
        self.fw_path_btn.configure(state="normal")
        self.fw_path_cb()
        self.pb.configure(value=0)

        if self.args.close_after:
            self.root.destroy()

    def _got_com(self, name, is_programming):
        # If this COM was just plugged in, it needs a moment to start existing
        time.sleep(0.2)

        if not is_programming:
            self.status.configure(text="Controller found. Switching to programming mode")
            serial.Serial(name, 1200).close()

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

        self.status.configure(text=f"Using {name} for update")
        # TODO: Basic sanity checks on the bin file
        cmd = build_update_command(name, self.fw_path.get())

        with subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        ) as p:
            if p.stdout:
                for line in p.stdout:
                    match = BOSSAC_PROGRESS_RE.match(line)
                    print(line, end="", flush=True)
                    if match:
                        self.pb.configure(value=(int(match.group(1)) / int(match.group(2))) * 100)
                    elif line.strip():
                        self.status.configure(text=line.strip())
                    self.root.update()
                for line in p.stderr:
                    self.status.configure(text=line)
                    print(line, end="", flush=True)
                    self.root.update()
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
        self.status.configure(text="Waiting for controller...")
        self.fw_path.configure(state="disabled")
        self.fw_path_btn.configure(state="disabled")
        self.flash_btn.configure(state="disabled")
        self.root.after(0, self._com_check)


if __name__ == "__main__":
    Flasher().root.mainloop()
