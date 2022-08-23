import binascii
import math
import struct
import sys
import time

import serial

from util import SAM_BA_BAUDRATE, find_port


BOOTLOADER_START = 0x0000
BOOTLOADER_END = 0x2000
WRITE_BUFFER = 0x20005000

NVM_UP_ADDR = 0x804000
NVM_UP_BOD33_DISABLE_OFFSET = 0x0
NVM_UP_BOD33_DISABLE_MASK = 0x1
NVM_UP_BOD33_RESET_OFFSET = 0x1
NVM_UP_BOD33_RESET_MASK = 0x2
NVM_UP_NVM_LOCK_OFFSET = 0x8


crc16_table = [
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
]


def crc16(data):
    crc16 = 0
    for i in data:
        crc16 = ((crc16 << 8) & 0xffff) ^ crc16_table[((crc16 >> 8) ^ i)]
    return crc16


class SAM_BA:
    def __init__(self, port):
        if isinstance(port, serial.Serial):
            self.com = port
        else:
            self.com = serial.Serial(port, SAM_BA_BAUDRATE)
        self._interactive = True
        self.interactive = False

    @property
    def interactive(self):
        return self._interactive

    @interactive.setter
    def interactive(self, val):
        if val == self._interactive:
            return
        self._interactive = val

        if val:
            self.com.write(b"T#")
            # TODO: Read CRLF+prompt
        else:
            self.com.write(b"N#\n")
            assert self.com.read(2) == b"\n\r"

    def get_version(self):
        self.com.write(b"V#")
        ver = b""
        while not ver.endswith(b"\n\r"):
            ver += self.com.read(1)
        return ver[:-2].decode("latin-1")

    def write_word(self, address, word):
        self.com.write(f"W{address:08x},{word:08x}#".encode())

    def write_half_word(self, address, half_word):
        self.com.write(f"H{address:08x},{half_word:04x}#".encode())

    def write_octet(self, address, octet):
        self.com.write(f"O{address:08x},{octet:02x}#".encode())

    def _write(self, address, data):
        self.com.write(f"S{address:08x},{len(data):08x}#".encode())
        self.com.flush()
        self.com.write(data)

    def write(self, base, data, chunk_size=0x1000, callback=None):
        total = len(data)
        start = time.perf_counter()
        address = base
        while data:
            chunk = data[:chunk_size]
            data = data[chunk_size:]

            self._write(WRITE_BUFFER, chunk)
            self.commit_buffer(WRITE_BUFFER, address, len(chunk))
            address += len(chunk)

            written = total - len(data)

            if callback:
                callback(written, total)

            w = 30
            print(
                f"Writing to {base:08x}"
                f" [{'=' * round(w * written / total)}{' ' * (w - round(w * written / total))}]"
                f" {round((written / total) * 100)}%"
                f" ({math.ceil(written / 64)}/{math.ceil(total / 64)} pages)",
                end="\r", flush=True
            )
        print("")
        delta = time.perf_counter() - start
        print(f"Wrote {total} bytes in {delta:.5f}s ({total / delta / 1024:.3f} KiB/s)")

    def write_program(self, data, callback=None):
        print("Writing firmware...")
        self.write(BOOTLOADER_END, data, callback=callback)
        print("Validating firmware...")
        self.validate_checksum(BOOTLOADER_END, data)

    def commit_buffer(self, src, dst, size):
        self.com.write(f"Y{src:08x},0#".encode())
        assert self.com.read(3) == b"Y\n\r"
        self.com.write(f"Y{dst:08x},{size:08x}#".encode())
        assert self.com.read(3) == b"Y\n\r"

    def read_word(self, address):
        self.com.write(f"w{address:08x},4#".encode())
        return struct.unpack("<I", self.com.read(4))[0]

    def read_half_word(self, address):
        self.com.write(f"h{address:08x},2#".encode())
        return struct.unpack("<H", self.com.read(2))[0]

    def read_octet(self, address):
        self.com.write(f"o{address:08x},1#".encode())
        return self.com.read(1)[0]

    def _read(self, address, nbytes):
        self.com.write(f"R{address:08x},{nbytes:08x}#".encode())
        return self.com.read(nbytes)

    def read(self, address, nbytes):
        data = b""
        while nbytes:
            chunk = min(nbytes, 63)
            data += self._read(address, chunk)
            address += chunk
            nbytes -= chunk

        return data

    def calculate_checksum(self, address, nbytes):
        self.com.write(f"Z{address:08x},{nbytes:08x}#".encode())
        assert self.com.read(1) == b"Z"
        chk = struct.unpack(">I", binascii.unhexlify(self.com.read(8)))[0]
        assert self.com.read(3) == b"#\n\r"
        return chk

    def validate_checksum(self, address, data):
        chip_chk = self.calculate_checksum(address, len(data))
        local_chk = crc16(data)
        if chip_chk != local_chk:
            raise ValueError(f"Checksum missmatch (expected {local_chk:04x}, got {chip_chk:04x})")

    def chip_erase(self, address=BOOTLOADER_END):
        self.com.write(f"X{address:08x}#".encode())
        assert self.com.read(3) == b"X\n\r"

    def close(self):
        if self.com:
            self.com.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.close()


class SAM_BA_NVM(SAM_BA):
    def get_lock_regions(self):
        addr = NVM_UP_ADDR + NVM_UP_NVM_LOCK_OFFSET
        _lockRegions = 16
        lock = [False] * _lockRegions

        lockBits = 0
        for region in range(_lockRegions):
            if region % 8 == 0:
                lockBits = self.read_octet(addr)
                addr += 1
            lock[region] = (lockBits & (1 << (region % 8))) == 0

        return lock


class SAMD21(SAM_BA_NVM):
    @property
    def device_id(self):
        return self.read_word(0x41002018)

    def reset(self):
        self.write_word(0xe000ed0c, 0x05fa0004)
        self.com = None


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <firmware.bin>")
        exit()

    port = find_port()
    if port:
        print("Rebooting into bootloader")
        serial.Serial(port, 1200).close()
        time.sleep(3)

    port = find_port(True)
    if port is None:
        print("No programming ports")
        exit()

    firmware = open(sys.argv[1], "rb").read()

    sam = SAMD21(port)

    print("Found chip:")
    print("Bootloader:\t", sam.get_version())
    print("Chip ID:\t", hex(sam.device_id))

    print("Region locks:\t", "".join("L" if locked else "." for locked in sam.get_lock_regions()))

    print("Erasing chip...")
    sam.chip_erase()

    sam.write_program(firmware)

    print("Restarting...")
    sam.reset()
