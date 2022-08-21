#pragma once
#include <vendor.h>

#include "leds.h"
#include "pins.h"

typedef enum : uint8_t {
    con_mode_mixed = 0,
    con_mode_kb_mouse,
    con_mode_joystick_position,
    con_mode_joystick_direction,
    _no_con_modes,
} con_mode_t;

constexpr uint8_t PERSIST_DATA_VERSION = 1;

typedef struct {
    uint8_t delay;
    char keys[32];
} large_macro_t;
typedef struct {
    uint8_t delay;
    char keys[10];
} short_macro_t;

typedef struct {
    led_mode_config_t led_mode;
    bool auto_hid;
    bool reactive_buttons;
    con_mode_t con_mode;
    char keymap[len(PinConf::buttons)];

    led_button_mode_t button_lights;
    CHSV zone_colours[6];
    led_zone_mode_t zone_modes[6];

    large_macro_t large_macros[2];
    short_macro_t short_macros[4];
    uint8_t tiny_macro_speed;
    uint8_t macro_layer[len(PinConf::buttons)];

    uint16_t led_dim;
    uint16_t led_timeout;
    uint8_t led_brightness;
} persistent_data_t;

extern const persistent_data_t default_con_state;
extern persistent_data_t con_state;

void load_con_state();
void save_con_state();
