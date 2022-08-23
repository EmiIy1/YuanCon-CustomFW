import os
import sys
from typing import Optional
import serial.tools.list_ports


# VID/PID pair used by official konami controllers, spoofed for EAC
KONAMI_VID_PID = (0x1ccf, 0x101c)
# VID/PID pair of the Sparkfun bootloader
BOOTLOADER_VID_PID = (0x1b4f, 0x8d21)
# VID/PID pair allocated for the custom firmware
CFW_VID_PID = (0x04D8, 0xE72E)

RESET_BAUDRATE = 1200
CONFIG_BAUDRATE = 250000
SAM_BA_BAUDRATE = 921600

SAMD21_DEVICE_ID = 0x10010305

KEYBOARD_MODE = 1
MOUSE_MODE = 2
GAMEPAD_MODE_POS = 4
GAMEPAD_MODE_DIR = 8
GAMEPAD_MODE = GAMEPAD_MODE_POS | GAMEPAD_MODE_DIR


def real_path(path):
    if getattr(sys, "frozen", False):
        return os.path.join(sys._MEIPASS, path)
    return os.path.join(os.path.dirname(__file__), path)


def find_port(programming=False) -> Optional[str]:
    devices = serial.tools.list_ports.comports()

    if programming:
        for i in devices:
            if (i.vid, i.pid) == BOOTLOADER_VID_PID:
                return i.name
    else:
        for i in devices:
            if (i.vid, i.pid) == CFW_VID_PID:
                return i.name

        # We didn't find a CFW image. Can we find a con spoofing a konami one?
        for i in devices:
            if (i.vid, i.pid) == KONAMI_VID_PID:
                return i.name

    return None


def get_com_serial(com):
    devices = serial.tools.list_ports.comports()

    for i in devices:
        if i.name == com.name:
            return i.serial_number

    return ""


__all__ = (
    "KONAMI_VID_PID", "BOOTLOADER_VID_PID", "CFW_VID_PID",
    "RESET_BAUDRATE", "CONFIG_BAUDRATE", "SAM_BA_BAUDRATE",
    "real_path", "find_port", "get_com_serial",
)
