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
    MiniMouse.move(VolX.delta, VolY.delta);
    VolX.ticked();
    VolY.ticked();
}

constexpr float encoder_ppr = 360.0;
constexpr float encoder_scale = 0xffff / (encoder_ppr * 2);

void do_joystick(bool absolute) {
    if (absolute) {
        MiniGamepad.report.vol_x = VolX.val * encoder_scale;
        MiniGamepad.report.vol_y = VolY.val * encoder_scale;
    } else {
        MiniGamepad.report.vol_x = VolX.dir > 0 ? 0x7fff : VolX.dir < 0 ? -0x8000 : 0;
        MiniGamepad.report.vol_y = VolY.dir > 0 ? 0x7fff : VolY.dir < 0 ? -0x8000 : 0;
    }

    VolX.dir = VolY.dir = 0;

    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) MiniGamepad.report.buttons |= 1 << PinConf::gamepad_map[i];
        if (negedge_buttons & (1 << i))
            MiniGamepad.report.buttons &= ~(1 << PinConf::gamepad_map[i]);
    }

    MiniGamepad.write();
}
