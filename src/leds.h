#pragma once
#include <vendor.h>

typedef enum : uint8_t {
    led_laser_mode_white = 0,
    led_laser_mode_colour,
    led_laser_mode_none,
    _no_led_laser_modes,
} led_laser_mode_t;
typedef enum : uint8_t {
    led_zone_mode_rainbow = 0,
    led_zone_mode_colour,
    led_zone_mode_hid,
    led_zone_mode_none,
    _no_led_zone_modes,
} led_zone_mode_t;
typedef enum : uint8_t {
    led_button_mode_live = 0,
    led_button_mode_hid,
    led_button_mode_mixed,
    led_button_mode_none,
    _no_led_button_modes,
} led_button_mode_t;
typedef struct {
    led_laser_mode_t lasers;
} led_mode_config_t;

/*
The LED zones are:

() 0   ^   3 ()
   1 [BTs] 4
   2 [FXs] 5
*/
#define LedZoneSL 0
#define LedZoneWTL 1
#define LedZoneWBL 2
#define LedZoneSR 3
#define LedZoneWTR 4
#define LedZoneWBR 5
constexpr uint8_t hid_zones[] = { 4, 0, 1, 5, 2, 3 };

#define LED_has_colour()                                   \
    (con_state.led_mode.lasers == led_laser_mode_colour || \
     con_state.zone_modes[0] == led_zone_mode_colour ||    \
     con_state.zone_modes[1] == led_zone_mode_colour ||    \
     con_state.zone_modes[2] == led_zone_mode_colour ||    \
     con_state.zone_modes[3] == led_zone_mode_colour ||    \
     con_state.zone_modes[4] == led_zone_mode_colour ||    \
     con_state.zone_modes[5] == led_zone_mode_colour)

#define AutoHidOn() (con_state.auto_hid && last_hid && (millis() - last_hid < AUTO_HID_TIMEOUT))

extern uint32_t last_interaction;
#define LEDTimeout() \
    (con_state.led_timeout != 0xffff && (millis() - last_interaction) / 1000 > con_state.led_timeout)
#define LEDShouldDim() \
    (con_state.led_dim != 0xffff && (millis() - last_interaction) / 1000 > con_state.led_dim)

#define LEDs_are_off()                                                  \
    ((con_state.led_mode.lasers == led_laser_mode_none &&               \
      con_state.zone_modes[0] == led_zone_mode_none &&                  \
      con_state.zone_modes[1] == led_zone_mode_none &&                  \
      con_state.zone_modes[2] == led_zone_mode_none &&                  \
      con_state.zone_modes[3] == led_zone_mode_none &&                  \
      con_state.zone_modes[4] == led_zone_mode_none &&                  \
      con_state.zone_modes[5] == led_zone_mode_none && !AutoHidOn()) || \
     LEDTimeout())

extern uint16_t button_leds;

void setup_leds();
void blank_led();
void do_leds();
void do_button_leds();
void write_button_leds();
