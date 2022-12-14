#pragma once
#include <vendor.h>

#include "analog.h"
#include "leds.h"
#include "pins.h"

constexpr uint8_t CON_MODE_KEYBOARD = 1;
constexpr uint8_t CON_MODE_MOUSE = 2;
constexpr uint8_t CON_MODE_GAMEPAD_POS = 4;
constexpr uint8_t CON_MODE_GAMEPAD_DIR = 8;
constexpr uint8_t CON_MODE_GAMEPAD = CON_MODE_GAMEPAD_POS | CON_MODE_GAMEPAD_DIR;

constexpr uint8_t con_mode_mixed = CON_MODE_KEYBOARD | CON_MODE_MOUSE | CON_MODE_GAMEPAD_POS;
constexpr uint8_t con_mode_kb_mouse = CON_MODE_KEYBOARD | CON_MODE_MOUSE;
constexpr uint8_t con_mode_joystick_position = CON_MODE_GAMEPAD_POS;
constexpr uint8_t con_mode_joystick_direction = CON_MODE_GAMEPAD_DIR;

typedef struct {
    uint16_t mask;
    uint16_t rainbow_on;
    uint16_t rainbow_off;
    CRGB colours_on[NUM_BUTTONS];
    CRGB colours_off[NUM_BUTTONS];
} minty_config_t;

typedef struct {
    led_mode_config_t led_mode;
    bool auto_hid;
    bool reactive_buttons;
    uint8_t con_mode;
    char keymap[NUM_BUTTONS];
    uint8_t gamepad_map[NUM_BUTTONS];

    led_button_mode_t button_lights;
    CHSV zone_colours[NUM_LED_ZONES];
    uint8_t saturation;
    led_zone_mode_t zone_modes[NUM_LED_ZONES];
    minty_config_t minty_config;

    uint8_t macro_layer[NUM_BUTTONS];

    uint16_t led_dim;
    uint16_t led_timeout;
    uint8_t led_brightness;

    struct {
        uint16_t deadzone;
        uint8_t deadzone_bounceback;
        uint16_t bounceback_timer;
    } analogs[NUM_ANALOGS];

    struct {
        // ! See comment by MACRO_BYTES
        uint8_t macro_addresses[NUM_MACROS];
        uint8_t data[MACRO_BYTES];
    } macros;
} persistent_data_t;

extern const persistent_data_t default_con_state;
extern persistent_data_t con_state;

void load_con_state();
void save_con_state();
