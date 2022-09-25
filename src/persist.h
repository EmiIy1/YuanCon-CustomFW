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

constexpr uint8_t PERSIST_DATA_VERSION = 2;

typedef struct {
    uint8_t delay;
    char keys[32];
} large_macro_t;
typedef struct {
    uint8_t delay;
    uint8_t keys[10];
} short_macro_t;

typedef struct {
    led_mode_config_t led_mode;
    bool auto_hid;
    bool reactive_buttons;
    uint8_t con_mode;
    char keymap[len(PinConf::buttons)];
    uint8_t gamepad_map[len(PinConf::buttons)];

    led_button_mode_t button_lights;
    CHSV zone_colours[6];
    uint8_t saturation;
    led_zone_mode_t zone_modes[6];

    large_macro_t large_macros[2];
    short_macro_t short_macros[4];
    uint8_t tiny_macro_speed;
    uint8_t macro_layer[len(PinConf::buttons)];

    uint16_t led_dim;
    uint16_t led_timeout;
    uint8_t led_brightness;

    uint8_t num_analogs;
    struct {
        uint16_t deadzone;
        uint8_t deadzone_bounceback;
        uint16_t bounceback_timer;
    } analogs[NUM_ANALOGS];
} persistent_data_t;

extern const persistent_data_t default_con_state;
extern persistent_data_t con_state;

void load_con_state();
void save_con_state();
