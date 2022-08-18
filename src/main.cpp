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

unsigned long nvm_save_requested = 0;
constexpr unsigned long nvm_save_delay = 5000;

constexpr bool allowed_quick_mode_change = true;

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
        con_mode_t last_con_mode = con_state.con_mode;

        button_leds = 0;
        if (blink_tick & 4) button_leds |= (1 << con_state.con_mode);

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
        }
    } else if (buttons & KeyMap::led_mode) {
        if (posedge_buttons & KeyMap::change_wing) {
            if ((++*((uint8_t *)&con_state.led_mode.wing_upper)) == _no_led_wing_modes)
                con_state.led_mode.wing_upper = (led_wing_mode_t)0;

            con_state.led_mode.wing_lower = con_state.led_mode.wing_upper;
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

        button_leds = 0;
        if (buttons & KeyMap::change_wing) button_leds |= (1 << con_state.led_mode.wing_upper);
        if (buttons & KeyMap::change_start) button_leds |= (1 << con_state.led_mode.start);
        if (buttons & KeyMap::change_buttons) button_leds |= (1 << con_state.led_mode.buttons);
        if (buttons & KeyMap::change_lasers) button_leds |= (1 << con_state.led_mode.lasers);

        if (con_state.auto_hid) button_leds |= KeyMap::toggle_auto_hid;
        if (con_state.reactive_buttons) button_leds |= KeyMap::toggle_reactive_buttons;
    }

    static uint8_t led_change_tick = 0;
    // 2x slower for more precision
    if ((++led_change_tick) == 1) {
        led_change_tick = 0;

        if (!(buttons & KeyMap::led_mode) && buttons & KeyMap::led_colour && LED_has_colour()) {
            // TODO: Come up with a way to change all three zones without PC configuration
            con_state.led_solid_l[0].h += VolX.dir;
            con_state.led_solid_l[1] = con_state.led_solid_l[0];
            con_state.led_solid_l[2] = con_state.led_solid_l[0];
            con_state.led_solid_r[0].h += VolY.dir;
            con_state.led_solid_r[1] = con_state.led_solid_r[0];
            con_state.led_solid_r[2] = con_state.led_solid_r[0];
        }
    }

    // When writing we do a check if the blob is byte for byte identical and skip it if so, so we
    // don't need to be _super_ cautious here.
    if (!(buttons & KeyMap::led_mode) && negedge_buttons & KeyMap::led_colour && LED_has_colour())
        nvm_save_requested = micros();
    if (negedge_buttons & KeyMap::led_mode) nvm_save_requested = micros();
}

void handle_macro_keys() {
    // TODO: Easy user-definable macros

    // Light up the buttons that have macros assigned
    button_leds = PinConf::BT_A | PinConf::BT_B;

    if (posedge_buttons & PinConf::BT_A) {
        MiniKeyboard.press(KEYPAD_ADD);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.release(KEYPAD_ADD);
        MiniKeyboard.write();
    }

    if (posedge_buttons & PinConf::BT_B) {
        MiniKeyboard.press(KEYPAD_1);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.release(KEYPAD_1);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.press(KEYPAD_2);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.release(KEYPAD_2);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.press(KEYPAD_3);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.release(KEYPAD_3);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.press(KEYPAD_4);
        MiniKeyboard.write();
        delay(100);
        MiniKeyboard.release(KEYPAD_4);
        MiniKeyboard.write();
    }

    // Decrease sens by a lot
    static uint8_t tick = 0;
    if ((++tick) == 20) {
        tick = 0;
        if (VolX.dir < 0) MiniConsumer.write(MEDIA_VOLUME_DOWN);
        if (VolX.dir > 0) MiniConsumer.write(MEDIA_VOLUME_UP);
    }

    static uint8_t tick2 = 0;
    if ((++tick2) == 3) {
        tick2 = 0;
        // LEDs aren't changed linearly because the brightness isn't perceived that way
        if (VolY.dir < 0) {
            if (led_brightness) {
                if (led_brightness < 31)
                    led_brightness--;
                else if (led_brightness < 64)
                    led_brightness -= 2;
                else if (led_brightness < 127)
                    led_brightness -= 4;
                else
                    led_brightness -= 8;
            }
            if (led_brightness) led_brightness /= 1.1;
        }
        if (VolY.dir > 0) {
            if (led_brightness < 255) {
                if (led_brightness < 31)
                    led_brightness++;
                else if (led_brightness < 64)
                    led_brightness += 2;
                else if (led_brightness < 127)
                    led_brightness += 4;
                else {
                    led_brightness += 8;
                    if (led_brightness < 16)  // Wrapped
                        led_brightness = 255;
                }
            }
        }
    }
}

void setup() {
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        pinMode(PinConf::buttons[i].btn, INPUT_PULLUP);
        pinMode(PinConf::buttons[i].led, OUTPUT);
    }
    load_con_state();

    // setup_vol();
    setup_leds();

    read_buttons();
    // Changing modes requires also holding either of the FX keys, to make it a little harder to
    // accidentally do.
    if (buttons & (PinConf::FX_L | PinConf::FX_R)) {
        con_mode_t old_cm = con_state.con_mode;
        if (buttons & PinConf::BT_D) con_state.con_mode = con_mode_joystick_direction;
        if (buttons & PinConf::BT_C) con_state.con_mode = con_mode_joystick_position;
        if (buttons & PinConf::BT_B) con_state.con_mode = con_mode_kb_mouse;
        if (buttons & PinConf::BT_A) con_state.con_mode = con_mode_mixed;

        // ! Performs a write to flash. We perform this write immediatly, because the user has
        // no ! option to change their mind anyway.
        if (con_state.con_mode != old_cm) save_con_state();
    }

    SerialUSB.begin(115200);

    MiniGamepad.begin();
    HIDLeds.begin();
    MiniConsumer.begin();
    MiniMouse.begin();
    MiniKeyboard.begin();
}

void do_serial() {
    if (!SerialUSB.available()) return;
    switch (SerialUSB.read()) {
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
            SerialUSB.write(PERSIST_DATA_VERSION);
            SerialUSB.write(sizeof con_state);
            SerialUSB.write((uint8_t *)&con_state, sizeof con_state);
            break;
        case 'S':
            // Set board config
            SerialUSB.readBytes((uint8_t *)&con_state, sizeof con_state);
            SerialUSB.write('S');
            break;
        case 'c':
            // Clear board config
            memcpy(&con_state, &default_con_state, sizeof con_state);
            save_con_state();
            // Keymap possibly changed, so unpress everything
            MiniKeyboard.releaseAll();
            MiniKeyboard.write();
            // TODO: Unpress gamepad buttons, once those are configurable
            SerialUSB.write('c');
            break;
        case 'C':
            // Commit board config
            save_con_state();
            load_con_state();
            SerialUSB.write('C');
            break;
        default:
            break;
    }
}

void loop() {
    static unsigned long last_leds = micros();

    if (nvm_save_requested) {
        if ((micros() - nvm_save_requested) > nvm_save_delay) {
            // ! Performs write to flash
            save_con_state();

            nvm_save_requested = 0;
        }
    }

    if (SerialUSB) do_serial();
    read_buttons();

    handle_ex_buttons();
    if (buttons & KeyMap::macro_key) {
        handle_macro_keys();
    }

    vol_x_dir_led = VolX.dir;
    vol_y_dir_led = VolY.dir;

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
    // overkill to have 1000Hz LEDs. 100Hz is probably overkill too to be honest, but whatever.
    if (micros() - last_leds > 10000) {
        last_leds = micros();
        do_leds();
    }
}
