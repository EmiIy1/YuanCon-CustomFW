// #include "HID/Keymap.h"
#include <HID-project.h>

#include "HID/Lighting.h"
#include "HID/MiniConsumer.h"
#include "HID/MiniGamepad.h"
#include "HID/MiniKeyboard.h"
#include "HID/MiniMouse.h"
#include "buttons.h"
#include "kdb_mouse_joy.h"
#include "leds.h"
#include "persist.h"
#include "pins.h"
#include "vol.h"

void handle_ex_buttons() {
    static uint16_t blink_tick = 0;
    bool needs_save = false;

    if ((++blink_tick) == 512) blink_tick = 0;

    if (buttons == KeyMap::led_reset) {
        // As long as one, any, of the buttons was posedge
        if (posedge_buttons & KeyMap::led_reset) {
            memcpy(&con_state.led_mode, &builtin_modes[0], sizeof con_state.led_mode);
            needs_save = true;
        }
    } else if (buttons & KeyMap::led_mode) {
        button_leds = 0;

        if (buttons & PinConf::FX_L && buttons & PinConf::FX_R) {
            // Change controller mode
            con_mode_t last_con_mode = con_state.con_mode;

            if (posedge_buttons & PinConf::EX_2) {
                if ((++*((uint8_t *)&con_state.con_mode)) == _no_con_modes)
                    con_state.con_mode = (con_mode_t)0;
            }

            if (blink_tick & 64) button_leds |= (1 << con_state.con_mode);

            if (posedge_buttons & PinConf::BT_A) con_state.con_mode = con_mode_mixed;
            if (posedge_buttons & PinConf::BT_B) con_state.con_mode = con_mode_kb_mouse;
            if (posedge_buttons & PinConf::BT_C) con_state.con_mode = con_mode_joystick_position;
            if (posedge_buttons & PinConf::BT_D) con_state.con_mode = con_mode_joystick_direction;

            if (con_state.con_mode != last_con_mode) {
                MiniKeyboard.releaseAll();
                MiniKeyboard.write();

                MiniGamepad.report.buttons = 0;
                MiniGamepad.report.vol_x = 0;
                MiniGamepad.report.vol_y = 0;
                MiniGamepad.write();

                needs_save = true;
            }
        } else {
            if (posedge_buttons & KeyMap::change_wing) {
                if ((++*((uint8_t *)&con_state.led_mode.wing)) == _no_led_wing_modes)
                    con_state.led_mode.wing = (led_wing_mode_t)0;
            }
            if (posedge_buttons & KeyMap::change_start) {
                if ((++*((uint8_t *)&con_state.led_mode.start)) == _no_led_start_modes)
                    con_state.led_mode.start = (led_start_mode_t)0;
            }
            if (posedge_buttons & KeyMap::change_buttons) {
                if ((++*((uint8_t *)&con_state.led_mode.buttons)) == _no_led_button_modes)
                    con_state.led_mode.buttons = (led_button_mode_t)0;
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

            if (buttons & KeyMap::change_wing) button_leds |= (1 << con_state.led_mode.wing);
            if (buttons & KeyMap::change_start) button_leds |= (1 << con_state.led_mode.start);
            if (buttons & KeyMap::change_buttons) button_leds |= (1 << con_state.led_mode.buttons);
            if (buttons & KeyMap::change_lasers) button_leds |= (1 << con_state.led_mode.lasers);

            if (con_state.auto_hid) button_leds |= KeyMap::toggle_auto_hid;
            if (con_state.reactive_buttons) button_leds |= KeyMap::toggle_reactive_buttons;
        }
    }

    static uint8_t led_change_tick = 0;
    // 2x slower for more precision
    if ((++led_change_tick) == 1) {
        led_change_tick = 0;

        if (!(buttons & KeyMap::led_mode) && buttons & KeyMap::led_colour && LED_has_colour()) {
            // TODO: Come up with a way to change all three zones without PC configuration
            con_state.led_solid_l[0].h += vol_x_dir;
            con_state.led_solid_l[1] = con_state.led_solid_l[0];
            con_state.led_solid_l[2] = con_state.led_solid_l[0];
            con_state.led_solid_r[0].h += vol_y_dir;
            con_state.led_solid_r[1] = con_state.led_solid_r[0];
            con_state.led_solid_r[2] = con_state.led_solid_r[0];
        }
    }

    if (!(buttons & KeyMap::led_mode) && negedge_buttons & KeyMap::led_colour && LED_has_colour()) {
        needs_save = true;
    }
    if (negedge_buttons & KeyMap::led_mode) needs_save = true;

    if (needs_save) save_con_state();
}

void handle_macro_keys() {
    // TODO: Easy user-definable macros

    // Light up the buttons that have macros assigned
    button_leds = PinConf::BT_A | PinConf::BT_B;

    if (posedge_buttons & PinConf::BT_A) {
        MiniKeyboard.press(KEYPAD_ADD);
        delay(100);
        MiniKeyboard.release(KEYPAD_ADD);
    }
    if (posedge_buttons & PinConf::BT_B) {
        MiniKeyboard.press(KEYPAD_1);
        delay(100);
        MiniKeyboard.release(KEYPAD_1);
        delay(100);
        MiniKeyboard.press(KEYPAD_2);
        delay(100);
        MiniKeyboard.release(KEYPAD_2);
        delay(100);
        MiniKeyboard.press(KEYPAD_3);
        delay(100);
        MiniKeyboard.release(KEYPAD_3);
        delay(100);
        MiniKeyboard.press(KEYPAD_4);
        delay(100);
        MiniKeyboard.release(KEYPAD_4);
    }

    // Decrease sens by a lot
    static uint8_t tick = 0;
    if ((++tick) == 20) {
        tick = 0;
        if (vol_x_dir < 0) MiniConsumer.write(MEDIA_VOLUME_DOWN);
        if (vol_x_dir > 0) MiniConsumer.write(MEDIA_VOLUME_UP);
    }
}

void setup() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        pinMode(PinConf::buttons[i].btn, INPUT_PULLUP);
        pinMode(PinConf::buttons[i].led, OUTPUT);
    }
    load_con_state();

    setup_vol();
    setup_leds();

    HIDLeds.begin();
    MiniConsumer.begin();
    MiniGamepad.begin();
    MiniMouse.begin();
    MiniKeyboard.begin();

    Serial.begin(9600);
}

void do_serial() {
    if (!Serial.available()) return;
    switch (Serial.read()) {
        case 'R': {
            // Force a reboot into the bootloader by pretending we double tapped reset

            // https://github.com/sparkfun/Arduino_Boards/blob/682926ef72078d7939c12ea886f20e48cd901cd3/sparkfun/samd/bootloaders/zero/board_definitions_sparkfun_samd21dev.h#L38
            constexpr size_t BOOT_DOUBLE_TAP_ADDRESS = 0x20007FFCul;
            constexpr uint32_t DOUBLE_TAP_MAGIC = 0x07738135;
            *((uint32_t *)BOOT_DOUBLE_TAP_ADDRESS) = DOUBLE_TAP_MAGIC;
        }
        case 'r':
            // General reboot
            NVIC_SystemReset();
            break;
        case 's':
            // Get board config
            Serial.write(PERSIST_DATA_VERSION);
            Serial.write(sizeof con_state);
            Serial.write((uint8_t *)&con_state, sizeof con_state);
            break;
        case 'S':
            // Set board config
            Serial.readBytes((uint8_t *)&con_state, sizeof con_state);
            Serial.write('S');
            break;
        case 'c':
            // Clear board config
            memcpy(&con_state, &default_con_state, sizeof con_state);
            save_con_state();
            Serial.write('c');
            break;
        case 'C':
            // Commit board config
            save_con_state();
            load_con_state();
            Serial.write('C');
            break;
        default:
            break;
    }
}

void active_delay(unsigned long ms) {
    if (ms == 0) {
        return;
    }

    uint32_t start = micros();

    while (ms > 0) {
        // This line is the only edit compared to stock
        if (Serial) do_serial();

        yield();
        while (ms > 0 && (micros() - start) >= 1000) {
            ms--;
            start += 1000;
        }
    }
}

void loop() {
    static unsigned long last_leds = micros();

    read_buttons();

    handle_ex_buttons();
    if (buttons & KeyMap::macro_key) {
        handle_macro_keys();
    }

    vol_x_dir_led = vol_x_dir;
    vol_y_dir_led = vol_y_dir;

    switch (con_state.con_mode) {
        case con_mode_kb_mouse:
            do_keyboard();
            do_mouse();
            break;
        case con_mode_joystick_position:
            do_joystick(true);
            break;
        case con_mode_joystick_direction:
            do_joystick(false);
            break;
        case con_mode_mixed:
            do_keyboard();
            do_mouse();
            do_joystick(true);
            break;
        default:
            break;
    }

    write_button_leds();

    // We're only going to bother updating the LEDs every 10ms or so, because it's expensive and
    // overkill to have 1000Hz LEDs. 100Hz is probably over kill too to be honest, but whatever.
    if (micros() - last_leds > 10000) {
        last_leds = micros();
        do_leds();
    }
}
