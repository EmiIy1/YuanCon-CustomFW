#include <Arduino.h>
#include <HID-Project.h>

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

    Keyboard.begin();
    Mouse.begin();
    Gamepad.begin();
}

void do_keyboard() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (posedge_buttons & (1 << i)) Keyboard.press(PinConf::keymap[i]);
        if (negedge_buttons & (1 << i)) Keyboard.release(PinConf::keymap[i]);
    }
}

void do_mouse(bool mouse_accel) {
    static float accel_x = 1.0;
    static float accel_y = 1.0;
    constexpr float accel_max = 10.0;

    int8_t move_x = vol_x_dir, move_y = vol_y_dir;
    vol_x_dir = vol_y_dir = 0;

    if (mouse_accel) {
        if (move_x) {
            if (accel_x < accel_max) accel_x += 0.075;
        } else if (accel_x > 1.0)
            accel_x -= 0.5;
        else
            accel_x = 1.0;

        if (move_y) {
            if (accel_y < accel_max) accel_y += 0.075;
        } else if (accel_y > 1.0)
            accel_y -= 0.5;
        else
            accel_y = 1.0;

        if (move_x) move_x *= accel_x;
        if (move_y) move_y *= accel_y;
    }

    if (move_x || move_y) Mouse.move(move_x, move_y);
}

constexpr uint8_t axis_speed = 75;
void do_joystick(bool absolute) {
    static int16_t abs_x = 0, abs_y = 0;

    abs_x += vol_x_dir * axis_speed;
    abs_y += vol_y_dir * axis_speed;
    if (absolute) {
        Gamepad.xAxis(abs_x);
        Gamepad.yAxis(abs_y);
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

    if (buttons & PinConf::EX_1) {
        if (buttons & PinConf::FX_L && buttons & PinConf::FX_R) {
            // Change controller mode
            con_mode_t last_con_mode = con_mode;

            if (posedge_buttons & PinConf::EX_2) {
                if ((++*((uint8_t*)&con_mode)) == _no_con_modes) con_mode = (con_mode_t)0;
            }

            if (blink_tick & 64) digitalWrite(PinConf::buttons[(uint8_t)con_mode].led, HIGH);
            if (posedge_buttons & PinConf::BT_A) con_mode = con_mode_kb_mouse_constant;
            if (posedge_buttons & PinConf::BT_B) con_mode = con_mode_kb_mouse_accel;
            if (posedge_buttons & PinConf::BT_C) con_mode = con_mode_joystick_position;
            if (posedge_buttons & PinConf::BT_D) con_mode = con_mode_joystick_direction;

            if (con_mode != last_con_mode) {
                Keyboard.releaseAll();
                Gamepad.releaseAll();
                Gamepad.xAxis(0);
                Gamepad.yAxis(0);
                Gamepad.write();
            }
        } else {
            // Change LED mode
            if (posedge_buttons & PinConf::EX_2) {
                if ((++*((uint8_t*)&led_mode)) == _no_led_modes) led_mode = (led_mode_t)0;
            }

            if (blink_tick > 150) digitalWrite(PinConf::buttons[(uint8_t)led_mode].led, HIGH);
            if (posedge_buttons & PinConf::BT_A) led_mode = led_mode_rainbow;
            if (posedge_buttons & PinConf::BT_B) led_mode = led_mode_solid;
            if (posedge_buttons & PinConf::BT_C) led_mode = led_mode_lasers;
            if (posedge_buttons & PinConf::BT_D) led_mode = led_mode_none;
        }
    }

    static uint8_t led_change_tick = 0;
    // 3x slower for more precision
    if ((++led_change_tick) == 2) {
        led_change_tick = 0;

        if (buttons & PinConf::EX_1 &&
            (led_mode == led_mode_solid || led_mode == led_mode_lasers)) {
            led_solid_l.h += vol_x_dir;
            led_solid_r.h += vol_y_dir;
        }
    }
}

void loop() {
    static uint8_t tick = 0;

    read_buttons();
    write_button_leds();

    handle_ex_buttons();

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

    if ((++tick) == 8) {
        tick = 0;
        do_led_animation();
    }

    delayMicroseconds(1000);
}
