#include <FastLED.h>

typedef enum : uint8_t {
    led_mode_rainbow = 0,
    led_mode_solid,
    led_mode_lasers,
    led_mode_none,
    _no_led_modes,
} led_mode_t;

extern led_mode_t led_mode;
extern CHSV led_solid_l;
extern CHSV led_solid_r;

void setup_leds();
void do_led_animation();
