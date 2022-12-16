import os

CONFIG_PATH = "config.def"

if not os.path.exists(CONFIG_PATH):
    CONFIG_PATH = "../" + CONFIG_PATH

with open(os.path.join(os.path.dirname(__file__), CONFIG_PATH)) as config_file:
    for line in config_file:
        if not line.startswith("#define "):
            continue
        line = line[8:].split(" ", 1)
        val = line[1].strip()

        if val.isdigit():
            val = int(val)
        elif val.startswith('"') and val.endswith('"'):
            val = val[1:-1]
        elif val.startswith("'") and val.endswith("'") and len(val) == 3:
            val = ord(val[1])

        globals()[line[0]] = val
