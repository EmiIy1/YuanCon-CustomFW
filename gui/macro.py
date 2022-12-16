import enum


class MacroOp(enum.Enum):
    END = b"\x00"
    DELAY = b"\x01"
    PAUSE = b"\x02"
    SET_PAUSE = b"\x03"

    PRESS = b"\x10"
    PRESS_C = b"\x11"

    DOWN = b"\x20"
    DOWN_C = b"\x21"

    UP = b"\x30"
    UP_C = b"\x31"

    TYPE = b"\xff"  # pseudo-op


def compile_macro(macro):
    compiled = b""

    for line in macro.split("\n"):
        line = line.strip().split("//", 1)[0]
        if not line:
            continue

        line = line.split(" ", 1)
        op = line[0]

        try:
            op = MacroOp[op]
        except KeyError:
            return False, f"Unknown command: {op}"

        if op == MacroOp.END:
            break
        elif op == MacroOp.PAUSE:
            compiled += MacroOp.PAUSE.value
        elif op == MacroOp.UP_C:
            if len(line) == 1:
                return False, "UP_C requires an argument"
            compiled += MacroOp.UP_C.value + line[1][0].encode("latin-1")
        elif op == MacroOp.DOWN_C:
            if len(line) == 1:
                return False, "DOWN_C requires an argument"
            compiled += MacroOp.DOWN_C.value + line[1][0].encode("latin-1")
        elif op == MacroOp.PRESS_C:
            if len(line) == 1:
                return False, "PRESS_C requires an argument"
            compiled += MacroOp.PRESS_C.value + line[1][0].encode("latin-1")
        elif op == MacroOp.TYPE:
            if len(line) == 1:
                return False, "TYPE requires an argument"
            for i in line[1]:
                compiled += MacroOp.PRESS_C.value + i.encode("latin-1")
        elif op == MacroOp.DELAY:
            if len(line) == 1:
                return False, "DELAY requires an argument"
            if not line[1].isdigit() or not int(line[1]) > 0:
                return False, "DELAY's argument must be > 0"
            val = int(line[1])
            while val > 0:
                compiled += op.value + bytearray([int(min(255, val))])
                val -= 255
        else:
            if len(line) == 1:
                return False, f"{op.name} requires an argument"
            if not line[1].isdigit() or not 0 <= int(line[1]) <= 255:
                return False, f"{op.name}'s argument must be 0<=arg<=255"

            compiled += op.value + bytearray([int(line[1])])

    compiled += MacroOp.END.value
    return True, compiled


if __name__ == "__main__":
    print(compile_macro("""
SET_PAUSE 30
PRESS_C a
PRESS_C b
PRESS_C c
PRESS_C d
"""))
