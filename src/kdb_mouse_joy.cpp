#include "HID/MiniGamepad.h"
#include "HID/MiniKeyboard.h"
#include "HID/MiniMouse.h"
#include "analog.h"
#include "buttons.h"
#include "persist.h"
#include "pins.h"

void do_keyboard() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) MiniKeyboard.press(con_state.keymap[i]);
        if (negedge_buttons & (1 << i)) MiniKeyboard.release(con_state.keymap[i]);
    }

    MiniKeyboard.write();
}

void do_mouse() { MiniMouse.move(analog_inputs[0].delta, analog_inputs[1].delta); }

constexpr float encoder_ppr = 360.0;
constexpr float encoder_scale = 0xffff / (encoder_ppr * 2);

void do_joystick(bool absolute) {
    if (absolute) {
        MiniGamepad.report.vol_x = analog_inputs[0].val * encoder_scale;
        MiniGamepad.report.vol_y = analog_inputs[1].val * encoder_scale;
    } else {
        MiniGamepad.report.vol_x = vol_x_dir > 0 ? 0x7fff : vol_x_dir < 0 ? -0x8000 : 0;
        MiniGamepad.report.vol_y = vol_y_dir > 0 ? 0x7fff : vol_y_dir < 0 ? -0x8000 : 0;

        vol_x_dir = vol_y_dir = 0;
    }

    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) MiniGamepad.report.buttons |= 1 << con_state.gamepad_map[i];
        if (negedge_buttons & (1 << i))
            MiniGamepad.report.buttons &= ~(1 << con_state.gamepad_map[i]);
    }

    MiniGamepad.write();
}
