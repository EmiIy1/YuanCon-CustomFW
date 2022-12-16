#pragma once

#include <HID.h>
#include <vendor.h>

#include "Keymap.h"

constexpr uint8_t MiniKeyboard_Keys = 10 + 1;

typedef struct ATTRIBUTE_PACKED {
    uint8_t buttons[MiniKeyboard_Keys];
    union {
        struct {
            bool ctrl : 1;
            bool shift : 1;
            bool alt : 1;
            bool gui : 1;
            bool r_ctrl : 1;
            bool r_shift : 1;
            bool r_alt : 1;
            bool r_gui : 1;
        };
        uint8_t modifiers;
    };
    uint8_t extra_button;
} HID_MiniKeyboardReport_Data_t;

class MiniKeyboard_ {
    uint8_t depressed;
    uint8_t modifier_count[8];
    bool dirty;

   public:
    HID_MiniKeyboardReport_Data_t report;

    MiniKeyboard_(void);
    void begin(void);
    int SendReport(void* data, int length);
    int write(void);

    void add_modifiers(uint8_t modifiers);
    void remove_modifiers(uint8_t modifiers);
    bool press(KeyboardKeycode key);
    bool press(uint8_t key);
    void release(KeyboardKeycode key);
    void release(uint8_t key);
    void releaseAll(void);
};

extern MiniKeyboard_ MiniKeyboard;
