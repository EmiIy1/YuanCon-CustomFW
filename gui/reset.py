import serial

from util import CONFIG_BAUDRATE, find_port


port = find_port()
com = serial.Serial(port, baudrate=CONFIG_BAUDRATE)

com.write(b'l')
assert com.read(1) == b'l'
