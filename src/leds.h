#pragma once
#include <vendor.h>

typedef enum : uint8_t {
    led_laser_mode_white = 0,
    led_laser_mode_colour,
    led_laser_mode_none,
    _no_led_laser_modes,
} led_laser_mode_t;
typedef enum : uint8_t {
    led_start_mode_rainbow = 0,
    led_start_mode_colour,
    led_start_mode_hid,
    led_start_mode_none,
    _no_led_start_modes,
} led_start_mode_t;
typedef enum : uint8_t {
    led_wing_mode_rainbow = 0,
    led_wing_mode_colour,
    led_wing_mode_hid,
    led_wing_mode_none,
    _no_led_wing_modes,
} led_wing_mode_t;
typedef enum : uint8_t {
    led_button_mode_live = 0,
    led_button_mode_hid,
    led_button_mode_mixed,
    led_button_mode_none,
    _no_led_button_modes,
} led_button_mode_t;
typedef struct {
    led_laser_mode_t lasers;
    led_start_mode_t start;
    led_wing_mode_t wing_upper;
    led_wing_mode_t wing_lower;
    led_button_mode_t buttons;
} led_mode_config_t;

extern led_mode_config_t builtin_modes[];
extern led_mode_config_t* led_quick_dial[4];

#define LED_has_colour()                                      \
    (con_state.led_mode.lasers == led_laser_mode_colour ||    \
     con_state.led_mode.start == led_start_mode_colour ||     \
     con_state.led_mode.wing_upper == led_wing_mode_colour || \
     con_state.led_mode.wing_lower == led_wing_mode_colour)

extern uint16_t button_leds;

void setup_leds();
void do_leds();
void write_button_leds();
