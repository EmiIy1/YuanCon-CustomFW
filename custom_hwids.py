Import("env")

board_config = env.BoardConfig()
board_config.update("build.hwids", [
  ["0x1ccf", "0x101c"],
  # ["0x1ccf", "0x1014"],
])
