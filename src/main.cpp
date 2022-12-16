#include "HID/Custom-HID.h"
#include "HID/Lighting.h"
#include "HID/MiniConsumer.h"
#include "HID/MiniGamepad.h"
#include "HID/MiniKeyboard.h"
#include "HID/MiniMouse.h"
#include "analog.h"
#include "buttons.h"
#include "kdb_mouse_joy.h"
#include "leds.h"
#include "macro.h"
#include "persist.h"
#include "pins.h"
#include "serial.h"

unsigned long nvm_save_requested = 0;
constexpr unsigned long nvm_save_delay = 5000;

constexpr bool allowed_quick_mode_change = true;

bool started_with_start = false;

void handle_con_mode_buttons() {
    uint8_t last_con_mode = con_state.con_mode;

    if (posedge_buttons & PinConf::BT_A) con_state.con_mode ^= CON_MODE_KEYBOARD;
    if (posedge_buttons & PinConf::BT_B) con_state.con_mode ^= CON_MODE_MOUSE;
    if (posedge_buttons & PinConf::BT_C) {
        if (con_state.con_mode & CON_MODE_GAMEPAD_POS) {
            con_state.con_mode &= ~CON_MODE_GAMEPAD;
        } else {
            con_state.con_mode &= ~CON_MODE_GAMEPAD;
            con_state.con_mode |= CON_MODE_GAMEPAD_POS;
        }
    }
    if (posedge_buttons & PinConf::BT_D) {
        if (con_state.con_mode & CON_MODE_GAMEPAD_DIR) {
            con_state.con_mode &= ~CON_MODE_GAMEPAD;
        } else {
            con_state.con_mode &= ~CON_MODE_GAMEPAD;
            con_state.con_mode |= CON_MODE_GAMEPAD_DIR;
        }
    }

    if (con_state.con_mode != last_con_mode) {
        MiniKeyboard.releaseAll();
        MiniKeyboard.write();

        MiniGamepad.report.buttons = 0;
        MiniGamepad.report.vol_x = 0;
        MiniGamepad.report.vol_y = 0;
        MiniGamepad.write();
    }

    // Currently just 4 bit, so lines up perfectly. This'll need made more complex if that ever
    // stops being the case.
    button_leds |= con_state.con_mode;
}

void handle_ex_buttons() {
    static uint16_t blink_tick = 0;

    if ((++blink_tick) == 512) blink_tick = 0;

    if (buttons == KeyMap::led_reset) {
        // As long as one, any, of the buttons was posedge
        // TODO: Bring this back
        // if (posedge_buttons & KeyMap::led_reset) {
        //     memcpy(&con_state.led_mode, &builtin_modes[0], sizeof con_state.led_mode);
        //     needs_save = true;
        // }
    } else if (allowed_quick_mode_change &&
               (buttons & KeyMap::change_mode) == KeyMap::change_mode) {
        // Change controller mode
        disable_led_modes = 1;
        button_leds = 0;
        handle_con_mode_buttons();
        if (blink_tick & 256) button_leds = 0;
    } else if (buttons & KeyMap::led_mode) {
        if (posedge_buttons & KeyMap::change_wing) {
            if ((++*((uint8_t *)&con_state.zone_modes[LedZoneWTL])) == _no_led_zone_modes)
                con_state.zone_modes[LedZoneWTL] = (led_zone_mode_t)0;

            con_state.zone_modes[LedZoneWTR] = con_state.zone_modes[LedZoneWTL];
            con_state.zone_modes[LedZoneWBL] = con_state.zone_modes[LedZoneWTL];
            con_state.zone_modes[LedZoneWBR] = con_state.zone_modes[LedZoneWTL];
        }
        if (posedge_buttons & KeyMap::change_start) {
            if ((++*((uint8_t *)&con_state.zone_modes[LedZoneSL])) == _no_led_zone_modes)
                con_state.zone_modes[LedZoneSL] = (led_zone_mode_t)0;

            con_state.zone_modes[LedZoneSR] = con_state.zone_modes[LedZoneSL];
        }
        if (posedge_buttons & KeyMap::change_buttons) {
            if ((++*((uint8_t *)&con_state.button_lights)) == _no_led_button_modes)
                con_state.button_lights = (led_button_mode_t)0;
        }
        if (posedge_buttons & KeyMap::change_lasers) {
            if ((++*((uint8_t *)&con_state.led_mode.lasers)) == _no_led_laser_modes)
                con_state.led_mode.lasers = (led_laser_mode_t)0;
        }

        if (posedge_buttons & KeyMap::toggle_auto_hid) {
            con_state.auto_hid = !con_state.auto_hid;
        }
        if (posedge_buttons & KeyMap::toggle_reactive_buttons) {
            con_state.reactive_buttons = !con_state.reactive_buttons;
        }

        disable_led_modes = 1;
        button_leds = 0;
        if (buttons & KeyMap::change_wing) button_leds |= (1 << con_state.zone_modes[LedZoneWTL]);
        if (buttons & KeyMap::change_start) button_leds |= (1 << con_state.zone_modes[LedZoneSL]);
        if (buttons & KeyMap::change_buttons) button_leds |= (1 << con_state.button_lights);
        if (buttons & KeyMap::change_lasers) button_leds |= (1 << con_state.led_mode.lasers);

        if (con_state.auto_hid) button_leds |= KeyMap::toggle_auto_hid;
        if (con_state.reactive_buttons) button_leds |= KeyMap::toggle_reactive_buttons;
    }

    static uint8_t led_change_tick = 0;
    // 2x slower for more precision
    if ((++led_change_tick) == 1) {
        led_change_tick = 0;

        if (!(buttons & KeyMap::led_mode) && buttons & KeyMap::led_colour && LED_has_colour()) {
            // TODO: Come up with a way to change all six zones without PC configuration
            con_state.zone_colours[0].h += analog_inputs[0].dir;
            con_state.zone_colours[1] = con_state.zone_colours[0];
            con_state.zone_colours[2] = con_state.zone_colours[0];
            con_state.zone_colours[3].h += analog_inputs[1].dir;
            con_state.zone_colours[4] = con_state.zone_colours[3];
            con_state.zone_colours[5] = con_state.zone_colours[3];
        }
    }

    // When writing we do a check if the blob is byte for byte identical and skip it if so, so we
    // don't need to be _super_ cautious here.
    if (!(buttons & KeyMap::led_mode) && negedge_buttons & KeyMap::led_colour && LED_has_colour())
        nvm_save_requested = micros();
    if (negedge_buttons & KeyMap::led_mode) nvm_save_requested = micros();
}

void handle_macro_keys() {
    disable_led_modes = 1;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        if (!con_state.macro_layer[i]) continue;

        // Light up the buttons that have macros assigned
        button_leds |= 1 << i;
        if (!(posedge_buttons & (1 << i))) continue;

        auto address = con_state.macros.macro_addresses[con_state.macro_layer[i] - 1];
        start_macro(&(con_state.macros.data[address]));
    }

    // Decrease sens by a lot
    static uint8_t tick = 0;
    if ((++tick) == 10) {
        tick = 0;
        if (analog_inputs[0].dir < 0) MiniConsumer.tap(MEDIA_VOLUME_DOWN);
        if (analog_inputs[0].dir > 0) MiniConsumer.tap(MEDIA_VOLUME_UP);
    }

    static uint8_t tick2 = 0;
    if ((++tick2) == 3) {
        tick2 = 0;
        // LEDs aren't changed linearly because the brightness isn't perceived that way
        if (analog_inputs[1].dir < 0) {
            if (con_state.led_brightness) {
                if (con_state.led_brightness < 31)
                    con_state.led_brightness--;
                else if (con_state.led_brightness < 64)
                    con_state.led_brightness -= 2;
                else if (con_state.led_brightness < 127)
                    con_state.led_brightness -= 4;
                else
                    con_state.led_brightness -= 8;
            }
            if (con_state.led_brightness) con_state.led_brightness /= 1.1;

            nvm_save_requested = micros();
        }
        if (analog_inputs[1].dir > 0) {
            if (con_state.led_brightness < 255) {
                if (con_state.led_brightness < 31)
                    con_state.led_brightness++;
                else if (con_state.led_brightness < 64)
                    con_state.led_brightness += 2;
                else if (con_state.led_brightness < 127)
                    con_state.led_brightness += 4;
                else {
                    con_state.led_brightness += 8;
                    if (con_state.led_brightness < 16)  // Wrapped
                        con_state.led_brightness = 255;
                }
            }

            nvm_save_requested = micros();
        }
    }
}

void setup() {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        pinMode(PinConf::buttons[i].btn, INPUT_PULLUP);
        pinMode(PinConf::buttons[i].led, OUTPUT);
    }
    load_con_state();

    setup_leds();

    read_buttons();
    started_with_start = !!(buttons & PinConf::START);
    // Changing modes requires also holding either of the FX keys, to make it a little harder to
    // accidentally do.
    // if (buttons & (PinConf::FX_L | PinConf::FX_R)) {
    //     uint8_t old_cm = con_state.con_mode;
    //     if (buttons & PinConf::BT_D) con_state.con_mode = con_mode_joystick_direction;
    //     if (buttons & PinConf::BT_C) con_state.con_mode = con_mode_joystick_position;
    //     if (buttons & PinConf::BT_B) con_state.con_mode = con_mode_kb_mouse;
    //     if (buttons & PinConf::BT_A) con_state.con_mode = con_mode_mixed;

    //     // ! Performs a write to flash. We perform this write immediatly, because the user has
    //     // no ! option to change their mind anyway.
    //     if (con_state.con_mode != old_cm) save_con_state();
    // }

    MiniGamepad.begin();
    HIDLeds.begin();
    MiniConsumer.begin();
    MiniMouse.begin();
    MiniKeyboard.begin();
}

void do_startup_mode_change() {
    blank_led();
    FastLED.show();

    uint8_t blink = 0;
    while (1) {
        read_buttons();

        button_leds = 0;
        disable_led_modes = 1;
        handle_con_mode_buttons();
        blink++;
        if (blink & 8)
            button_leds = 0;
        else
            button_leds |= PinConf::START;

        write_button_leds();
        do_leds();
        // We don't care about polling rate, so go with something more reasonable so the blink works
        delay(20);

        if (posedge_buttons & PinConf::START) {
            save_con_state();
            return;
        }
    }
}

void __attribute__((always_inline)) inline tick_read_inputs() {
    read_buttons();
    for (uint8_t i = 0; i < NUM_ANALOGS; i++) analog_inputs[i].tick();
    vol_x_dir_led = analog_inputs[0].dir;
    vol_y_dir_led = analog_inputs[1].dir;
    vol_x_dir = analog_inputs[0].dir;
    vol_y_dir = analog_inputs[1].dir;
}

void __attribute__((always_inline)) inline tick_process_inputs() {
    handle_ex_buttons();
    tick_all_macros();
    if (buttons & KeyMap::macro_key) {
        handle_macro_keys();
    } else {
        if (con_state.con_mode & CON_MODE_KEYBOARD)
            do_keyboard();
        else
            MiniKeyboard.releaseAll();
        if (con_state.con_mode & CON_MODE_MOUSE) do_mouse();
        if (con_state.con_mode & CON_MODE_GAMEPAD)
            do_joystick(!!(con_state.con_mode & CON_MODE_GAMEPAD_POS));
    }
}

void __attribute__((always_inline)) inline tick_write_usb() {
    // These two can be changed for many reasons, and will only actually write if dirty
    MiniMouse.write();
    MiniGamepad.write();
    MiniKeyboard.write();
    MiniConsumer.write();

    CustomHID().FlushReports();
    // CustomHID_Lighting().FlushReports();

    // We need to send something in response, but have nothing to send!
    if (CustomHID().sent == 0 && hid_need_ack)
        CustomHID().sent = 0;
    else
        CustomHID().sent = 0;
}

void __attribute__((always_inline)) inline tick_write_leds() {
    static unsigned long last_leds = micros();

    write_button_leds();

    // We're only going to bother updating the LEDs every 10ms or so, because it's expensive and
    // overkill to have 1000Hz LEDs. 100Hz is probably overkill too to be honest, but whatever.
    if (micros() - last_leds > 10000) {
        last_leds = micros();
        do_leds();
    }
}

void __attribute__((always_inline)) inline tick_startup_configuration() {
    if (!(buttons & PinConf::START))
        started_with_start = false;
    else if (millis() > 5000) {
        do_startup_mode_change();
    }

    // Flash start to indicate something is going on. It's light from holding it, so we need to
    // _un_ light it to make it flash.
    if (millis() & 128) button_leds &= ~(PinConf::START);
}

void loop() {
    // Non-volatile storage
    if (nvm_save_requested) {
        if ((micros() - nvm_save_requested) > nvm_save_delay) {
            // ! Performs write to flash
            save_con_state();

            nvm_save_requested = 0;
        }
    }

    // Serial configuration and SAM-BA
    if (SerialUSB.available()) tick_serial();

    // Read inputs
    tick_read_inputs();
    // Other things might want to mess with button_leds aftwards, so do this first
    do_button_leds();

    // We're in configuration mode still, so act accordingly
    if (started_with_start) tick_startup_configuration();

    tick_process_inputs();

    // Reset for the next tick
    analog_inputs[0].dir = analog_inputs[1].dir = 0;

    // Write out to USB
    // TODO: some of the do_* functions do an immediate write.
    // TODO: do we want to defer that to here for tidyness?
    tick_write_usb();

    tick_write_leds();
}
