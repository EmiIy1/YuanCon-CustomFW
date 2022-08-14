#include <Arduino.h>
#include <HID-Project.h>

#include "buttons.h"
#include "pins.h"
#include "vol.h"

void do_keyboard() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) NKROKeyboard.press(PinConf::keymap[i]);
        if (negedge_buttons & (1 << i)) NKROKeyboard.release(PinConf::keymap[i]);
    }
}

void do_mouse() {
    if (vol_delta_x || vol_delta_y) Mouse.move(vol_delta_x, vol_delta_y);
    vol_delta_x = vol_delta_y = 0;

    // Reset these for anything that uses them
    vol_x_dir = vol_y_dir = 0;
}

constexpr uint8_t axis_speed = 75;
void do_joystick(bool absolute) {
    if (absolute) {
        Gamepad.xAxis(vol_x * axis_speed);
        Gamepad.yAxis(vol_y * axis_speed);
    } else {
        Gamepad.xAxis(vol_x_dir > 0 ? 0x7fff : vol_x_dir < 0 ? -0x8000 : 0);
        Gamepad.yAxis(vol_y_dir > 0 ? 0x7fff : vol_y_dir < 0 ? -0x8000 : 0);
    }

    vol_x_dir = vol_y_dir = 0;

    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) Gamepad.press(PinConf::gamepad_map[i]);
        if (negedge_buttons & (1 << i)) Gamepad.release(PinConf::gamepad_map[i]);
    }

    Gamepad.write();
}
