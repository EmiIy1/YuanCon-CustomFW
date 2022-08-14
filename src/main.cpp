#include <Arduino.h>
#include <HID-Project.h>

#include "HID_lighting.h"
#include "buttons.h"
#include "leds.h"
#include "pins.h"
#include "vol.h"

typedef enum : uint8_t {
    con_mode_kb_mouse_constant = 0,
    con_mode_kb_mouse_accel,
    con_mode_joystick_position,
    con_mode_joystick_direction,
    _no_con_modes,
} con_mode_t;
con_mode_t con_mode = con_mode_kb_mouse_constant;

void setup() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        pinMode(PinConf::buttons[i].btn, INPUT_PULLUP);
        pinMode(PinConf::buttons[i].led, OUTPUT);
    }

    setup_vol();
    setup_leds();

    Consumer.begin();
    NKROKeyboard.begin();
    Mouse.begin();
    Gamepad.begin();
    HIDLeds.begin();
}

void do_keyboard() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) NKROKeyboard.press(PinConf::keymap[i]);
        if (negedge_buttons & (1 << i)) NKROKeyboard.release(PinConf::keymap[i]);
    }
}

void do_mouse(bool mouse_accel) {
    // static float accel_x = 0;
    // static float accel_y = 0;
    // constexpr float accel_max = 10.0;

    static uint16_t last_x = 0, last_y = 0;
    static unsigned long last_tick = 0;

    int8_t move_x = vol_x_dir, move_y = vol_y_dir;
    vol_x_dir = vol_y_dir = 0;

    if (mouse_accel) {
        unsigned long now = millis();
        auto dt = now - last_tick;

        if (last_tick != 0) {
            uint16_t dx = vol_x - last_x;
            uint16_t dy = vol_y - last_y;
        }
        last_x = vol_x;
        last_y = vol_y;

        last_tick = now;

        // if (move_x) {
        //     if (accel_x < accel_max) accel_x += 0.075;
        // } else if (accel_x > 1.0)
        //     accel_x -= 0.5;
        // else
        //     accel_x = 1.0;

        // if (move_y) {
        //     if (accel_y < accel_max) accel_y += 0.075;
        // } else if (accel_y > 1.0)
        //     accel_y -= 0.5;
        // else
        //     accel_y = 1.0;

        // if (move_x) move_x *= accel_x;
        // if (move_y) move_y *= accel_y;
    }

    if (move_x || move_y) Mouse.move(move_x, move_y);
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

void handle_ex_buttons() {
    static uint16_t blink_tick = 0;

    if ((++blink_tick) == 512) blink_tick = 0;

    if (buttons == KeyMap::led_reset) {
        memcpy(&led_mode, &builtin_modes[0], sizeof led_mode);
    } else if (buttons & KeyMap::led_mode) {
        button_leds = 0;

        if (buttons & PinConf::FX_L && buttons & PinConf::FX_R) {
            // Change controller mode
            con_mode_t last_con_mode = con_mode;

            if (posedge_buttons & PinConf::EX_2) {
                if ((++*((uint8_t*)&con_mode)) == _no_con_modes) con_mode = (con_mode_t)0;
            }

            if (blink_tick & 64) button_leds |= (1 << con_mode);

            if (posedge_buttons & PinConf::BT_A) con_mode = con_mode_kb_mouse_constant;
            if (posedge_buttons & PinConf::BT_B) con_mode = con_mode_kb_mouse_accel;
            if (posedge_buttons & PinConf::BT_C) con_mode = con_mode_joystick_position;
            if (posedge_buttons & PinConf::BT_D) con_mode = con_mode_joystick_direction;

            if (con_mode != last_con_mode) {
                NKROKeyboard.releaseAll();
                Gamepad.releaseAll();
                Gamepad.xAxis(0);
                Gamepad.yAxis(0);
                Gamepad.write();
            }
        } else {
            if (posedge_buttons & KeyMap::change_wing) {
                if ((++*((uint8_t*)&led_mode.wing)) == _no_led_wing_modes)
                    led_mode.wing = (led_wing_mode_t)0;
            }
            if (posedge_buttons & KeyMap::change_start) {
                if ((++*((uint8_t*)&led_mode.start)) == _no_led_start_modes)
                    led_mode.start = (led_start_mode_t)0;
            }
            if (posedge_buttons & KeyMap::change_buttons) {
                if ((++*((uint8_t*)&led_mode.buttons)) == _no_led_button_modes)
                    led_mode.buttons = (led_button_mode_t)0;
            }
            if (posedge_buttons & KeyMap::change_lasers) {
                if ((++*((uint8_t*)&led_mode.lasers)) == _no_led_laser_modes)
                    led_mode.lasers = (led_laser_mode_t)0;
            }

            if (posedge_buttons & KeyMap::toggle_auto_hid) {
                auto_hid = !auto_hid;
            }

            if (buttons & KeyMap::change_wing) button_leds |= (1 << led_mode.wing);
            if (buttons & KeyMap::change_start) button_leds |= (1 << led_mode.start);
            if (buttons & KeyMap::change_buttons) button_leds |= (1 << led_mode.buttons);
            if (buttons & KeyMap::change_lasers) button_leds |= (1 << led_mode.lasers);

            if (auto_hid) button_leds |= KeyMap::toggle_auto_hid;
        }
    }

    static uint8_t led_change_tick = 0;
    // 2x slower for more precision
    if ((++led_change_tick) == 1) {
        led_change_tick = 0;

        if (!(buttons & KeyMap::led_mode) && buttons & KeyMap::led_colour && LED_has_colour()) {
            led_solid_l.h += vol_x_dir;
            led_solid_r.h += vol_y_dir;
        }
    }
}

void handle_macro_keys() {
    // TODO: Easy user-definable macros

    // Light up the buttons that have macros assigned
    button_leds = PinConf::BT_A | PinConf::BT_B;

    if (posedge_buttons & PinConf::BT_A) {
        NKROKeyboard.press(KEYPAD_ADD);
        delay(100);
        NKROKeyboard.release(KEYPAD_ADD);
    }
    if (posedge_buttons & PinConf::BT_B) {
        NKROKeyboard.press(KEYPAD_1);
        delay(100);
        NKROKeyboard.release(KEYPAD_1);
        delay(100);
        NKROKeyboard.press(KEYPAD_2);
        delay(100);
        NKROKeyboard.release(KEYPAD_2);
        delay(100);
        NKROKeyboard.press(KEYPAD_3);
        delay(100);
        NKROKeyboard.release(KEYPAD_3);
        delay(100);
        NKROKeyboard.press(KEYPAD_4);
        delay(100);
        NKROKeyboard.release(KEYPAD_4);
    }

    // Decrease sens by a lot
    static uint8_t tick = 0;
    if ((++tick) == 32) {
        tick = 0;
        if (vol_x_dir < 0) Consumer.write(MEDIA_VOLUME_DOWN);
        if (vol_x_dir > 0) Consumer.write(MEDIA_VOLUME_UP);
    }
}

void loop() {
    static auto last_tick = micros();
    // static uint16_t tick = 0;

    read_buttons();
    do_leds();

    handle_ex_buttons();
    if (buttons & KeyMap::macro_key) {
        handle_macro_keys();
    }

    vol_x_dir_led = vol_x_dir;
    vol_y_dir_led = vol_y_dir;

    switch (con_mode) {
        case con_mode_kb_mouse_constant:
            do_keyboard();
            do_mouse(false);
            break;
        case con_mode_kb_mouse_accel:
            do_keyboard();
            do_mouse(true);
            break;
        case con_mode_joystick_position:
            do_joystick(true);
            break;
        case con_mode_joystick_direction:
            do_joystick(false);
            break;
        default:
            break;
    }

    write_button_leds();

    auto now = micros();
    auto delta = now - last_tick;
    if (delta < 1000) {
        delayMicroseconds(1000 - delta);
    } else {
        // TODO: figure out some way to report the number of dropped cycles
    }
    last_tick = now;
}
