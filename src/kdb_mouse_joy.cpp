#include "HID/MiniGamepad.h"
#include "HID/MiniMouse.h"
#include "HID/MiniKeyboard.h"
#include "buttons.h"
#include "persist.h"
#include "pins.h"
#include "vol.h"

void do_keyboard() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) MiniKeyboard.press(con_state.keymap[i]);
        if (negedge_buttons & (1 << i)) MiniKeyboard.release(con_state.keymap[i]);
    }

    MiniKeyboard.write();
}

void do_mouse() {
    MiniMouse.move(vol_delta_x, vol_delta_y);
    vol_delta_x = vol_delta_y = 0;

    // Reset these for anything that uses them
    vol_x_dir = vol_y_dir = 0;
}

constexpr uint8_t axis_speed = 75;
void do_joystick(bool absolute) {
    if (absolute) {
        MiniGamepad.report.vol_x = vol_x * axis_speed;
        MiniGamepad.report.vol_y = vol_y * axis_speed;
    } else {
        MiniGamepad.report.vol_x = vol_x_dir > 0 ? 0x7fff : vol_x_dir < 0 ? -0x8000 : 0;
        MiniGamepad.report.vol_y = vol_y_dir > 0 ? 0x7fff : vol_y_dir < 0 ? -0x8000 : 0;
    }

    vol_x_dir = vol_y_dir = 0;

    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) MiniGamepad.report.buttons |= 1 << PinConf::gamepad_map[i];
        if (negedge_buttons & (1 << i))
            MiniGamepad.report.buttons &= ~(1 << PinConf::gamepad_map[i]);
    }

    MiniGamepad.write();
}
