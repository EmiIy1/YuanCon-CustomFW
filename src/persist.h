#pragma once
#include <vendor.h>

#include "leds.h"
#include "pins.h"

typedef enum : uint8_t {
    con_mode_kb_mouse = 0,
    con_mode_joystick_position,
    con_mode_joystick_direction,
    _no_con_modes,
} con_mode_t;

constexpr uint8_t PERSIST_DATA_VERSION = 2;
typedef struct {
    led_mode_config_t led_mode;
    CHSV led_solid_l;
    CHSV led_solid_r;
    bool auto_hid;
    con_mode_t con_mode;
    char keymap[len(PinConf::buttons)];
} persistent_data_t;

extern const persistent_data_t default_con_state;
extern persistent_data_t con_state;

void load_con_state();
void save_con_state();
