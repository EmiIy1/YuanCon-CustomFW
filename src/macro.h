#include <vendor.h>

#include "HID/MiniKeyboard.h"

enum class MacroOp : uint8_t {
    END = 0x00,
    DELAY = 0x01,
    PAUSE = 0x02,
    SET_PAUSE = 0x03,

    PRESS = 0x10,
    PRESS_C = 0x11,

    DOWN = 0x20,
    DOWN_C = 0x21,

    UP = 0x30,
    UP_C = 0x31,
};

enum class MacroState : uint8_t {
    Running = 0,
    Delay,
    DelayC,
    Complete,
};

typedef struct _macro_state {
    struct _macro_state* next;
    uint8_t* stack;
    MacroState state;
    uint8_t delay;
    union {
        KeyboardKeycode pressed;
        uint8_t pressed_c;
    } info;
    unsigned long until;
} macro_state_t;

bool tick_macro(macro_state_t* macro);

void tick_all_macros();
void start_macro(uint8_t* macro);
