Import("env")

board_config = env.BoardConfig()
# should be array of VID:PID pairs
board_config.update("build.hwids", [
  ["0x1ccf", "0x101c"],
])

board_config.update("build.usb_product", "YuantCon")
