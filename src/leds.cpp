#include "leds.h"

#include "HID/Lighting.h"
#include "buttons.h"
#include "persist.h"
#include "pins.h"
#include "vol.h"

uint16_t button_leds;
uint8_t led_brightness = 255;

typedef union {
    CRGB leds[PinConf::wing_rgb_count];
    struct {
        CRGB bottom[PinConf::wing_rgb_lower];
        CRGB top[PinConf::wing_rgb_upper];
    };
} wing_leds_t;
wing_leds_t wing_rgb_l_leds;
wing_leds_t wing_rgb_r_leds;
CRGB start_rgb_l_leds[PinConf::start_rgb_count];
CRGB start_rgb_r_leds[PinConf::start_rgb_count];

void blank_led() {
    for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
        wing_rgb_l_leds.leds[i] = CRGB(0, 0, 0);
        wing_rgb_r_leds.leds[i] = CRGB(0, 0, 0);
    }
    for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
        start_rgb_l_leds[i] = CRGB(0, 0, 0);
        start_rgb_r_leds[i] = CRGB(0, 0, 0);
    }
}

void toward_zero(int8_t* v) {
    if (*v > 0) (*v)--;
    if (*v < 0) (*v)++;
}

void render_led_shoot_start(int8_t shoot, CHSV colour) {
    if (shoot > 0) {
        if (shoot > PinConf::start_rgb_count)
            start_rgb_l_leds[shoot - PinConf::start_rgb_count - 1] = colour;
        else
            start_rgb_r_leds[PinConf::start_rgb_count - shoot] = colour;
    }
    if (shoot < 0) {
        if (shoot < -PinConf::start_rgb_count)
            start_rgb_r_leds[-shoot - PinConf::start_rgb_count - 1] = colour;
        else
            start_rgb_l_leds[PinConf::start_rgb_count + shoot] = colour;
    }
}

template <int count>
void render_led_shoot_wing(int8_t shoot, CHSV colour, CRGB* leds) {
    if (shoot > 0) leds[count - shoot] = colour;
    if (shoot < 0) leds[-shoot - 1] = colour;
}

template <int count, bool invert>
void render_led_fill(int8_t fill, CHSV colour, CRGB* leds) {
#define idx(i) (invert ? count - (i)-1 : (i))

    if (fill > 0)
        for (uint8_t i = 0; i <= (count - fill); i++) leds[idx(i)] = colour;
    else if (fill < 0)
        for (uint8_t i = count + fill + 1; i < count; i++) leds[idx(i)] = colour;

#undef idx
}

void setup_leds() {
    FastLED.addLeds<WS2812, PinConf::wing_rgb_l, GRB>(wing_rgb_l_leds.leds,
                                                      PinConf::wing_rgb_count);
    FastLED.addLeds<WS2812, PinConf::wing_rgb_r, GRB>(wing_rgb_r_leds.leds,
                                                      PinConf::wing_rgb_count);
    FastLED.addLeds<WS2812, PinConf::start_rgb_l, GRB>(start_rgb_l_leds, PinConf::start_rgb_count);
    FastLED.addLeds<WS2812, PinConf::start_rgb_r, GRB>(start_rgb_r_leds, PinConf::start_rgb_count);
    blank_led();
    FastLED.show();
}

// The lights in the YuanCon are so powerful and plentiful that when all at max the USB port is
// unable to supply enough power for the optical encoders to also function. This is problematic.
//
// This is not normally an issue, as solid white is only possible in HID mode.
//
// Because of this, in HID mode all the body LEDs are run at quater brightness. Alternative, comment
// the line below to switch to only lighting every second LED.
#define POWER_SAVE_BRIGHTNESS

uint8_t led_animation_frame = 0;
void do_wing_upper_leds() {
    if (LED_has_colour() && buttons & KeyMap::led_colour) {
        for (uint8_t i = 0; i < PinConf::wing_rgb_upper; i++) {
            wing_rgb_l_leds.top[i] = con_state.led_solid_l[1];
            wing_rgb_r_leds.top[i] = con_state.led_solid_r[1];
        }
        return;
    }

    led_wing_mode_t mode = con_state.led_mode.wing_upper;
    if (AutoHidOn()) mode = led_wing_mode_hid;

    switch (mode) {
        case led_wing_mode_hid:
#ifdef POWER_SAVE_BRIGHTNESS
            for (uint8_t i = 0; i < PinConf::wing_rgb_upper; i++) {
                wing_rgb_l_leds.top[i].setRGB(hid_led_data.leds.rgb[0].r / 4,
                                              hid_led_data.leds.rgb[0].g / 4,
                                              hid_led_data.leds.rgb[0].b / 4);
                wing_rgb_r_leds.top[i].setRGB(hid_led_data.leds.rgb[1].r / 4,
                                              hid_led_data.leds.rgb[1].g / 4,
                                              hid_led_data.leds.rgb[1].b / 4);
            }
#else
            for (uint8_t i = 0; i < PinConf::wing_rgb_upper; i += 2) {
                memcpy(wing_rgb_l_leds.top[i].raw, hid_led_data.leds.rgb[0].raw, 3);
                memcpy(wing_rgb_r_leds.top[i].raw, hid_led_data.leds.rgb[1].raw, 3);
            }
#endif
            break;
        case led_wing_mode_rainbow:
            for (uint8_t i = 0; i < PinConf::wing_rgb_upper; i++) {
                wing_rgb_l_leds.top[i] = CHSV(
                    led_animation_frame + i * (256 / ((float)PinConf::wing_rgb_upper)), 255, 255);
                wing_rgb_r_leds.top[i] = CHSV(
                    led_animation_frame + i * (256 / ((float)PinConf::wing_rgb_upper)), 255, 255);
            }
            break;
        case led_wing_mode_colour:
            for (uint8_t i = 0; i < PinConf::wing_rgb_upper; i++) {
                wing_rgb_l_leds.top[i] = con_state.led_solid_l[1];
                wing_rgb_r_leds.top[i] = con_state.led_solid_r[1];
            }
            break;
        default:
            break;
    }
}
void do_wing_lower_leds() {
    if (LED_has_colour() && buttons & KeyMap::led_colour) {
        for (uint8_t i = 0; i < PinConf::wing_rgb_lower; i++) {
            wing_rgb_l_leds.bottom[i] = con_state.led_solid_l[2];
            wing_rgb_r_leds.bottom[i] = con_state.led_solid_r[2];
        }
        return;
    }

    led_wing_mode_t mode = con_state.led_mode.wing_lower;
    if (AutoHidOn()) mode = led_wing_mode_hid;

    switch (mode) {
        case led_wing_mode_hid:
#ifdef POWER_SAVE_BRIGHTNESS
            for (uint8_t i = 0; i < PinConf::wing_rgb_lower; i++) {
                wing_rgb_l_leds.bottom[i].setRGB(hid_led_data.leds.rgb[2].r / 4,
                                                 hid_led_data.leds.rgb[2].g / 4,
                                                 hid_led_data.leds.rgb[2].b / 4);
                wing_rgb_r_leds.bottom[i].setRGB(hid_led_data.leds.rgb[3].r / 4,
                                                 hid_led_data.leds.rgb[3].g / 4,
                                                 hid_led_data.leds.rgb[3].b / 4);
            }
#else
            for (uint8_t i = 0; i < PinConf::wing_rgb_lower; i += 2) {
                memcpy(wing_rgb_l_leds.bottom[i].raw, hid_led_data.leds.rgb[2].raw, 3);
                memcpy(wing_rgb_r_leds.bottom[i].raw, hid_led_data.leds.rgb[3].raw, 3);
            }
#endif
            break;
        case led_wing_mode_rainbow:
            for (uint8_t i = 0; i < PinConf::wing_rgb_lower; i++) {
                wing_rgb_l_leds.bottom[PinConf::wing_rgb_lower - i - 1] = CHSV(
                    led_animation_frame + i * (256 / ((float)(PinConf::wing_rgb_lower))), 255, 255);
                wing_rgb_r_leds.bottom[PinConf::wing_rgb_lower - i - 1] = CHSV(
                    led_animation_frame + i * (256 / ((float)(PinConf::wing_rgb_lower))), 255, 255);
            }
            break;
        case led_wing_mode_colour:
            for (uint8_t i = 0; i < PinConf::wing_rgb_lower; i++) {
                wing_rgb_l_leds.bottom[i] = con_state.led_solid_l[2];
                wing_rgb_r_leds.bottom[i] = con_state.led_solid_r[2];
            }
            break;
        default:
            break;
    }
}

void do_start_leds() {
    if (LED_has_colour() && buttons & KeyMap::led_colour) {
        for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
            start_rgb_l_leds[i] = con_state.led_solid_l[0];
            start_rgb_r_leds[i] = con_state.led_solid_r[0];
        }
        return;
    }

    led_start_mode_t mode = con_state.led_mode.start;
    if (AutoHidOn()) mode = led_start_mode_hid;

    switch (mode) {
        case led_start_mode_rainbow:
            for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
                start_rgb_l_leds[PinConf::start_rgb_count - i - 1] =
                    CHSV(led_animation_frame + i * (256 / ((float)(PinConf::start_rgb_count))), 255,
                         255);
                start_rgb_r_leds[PinConf::start_rgb_count - i - 1] =
                    CHSV(led_animation_frame + i * (256 / ((float)(PinConf::start_rgb_count))), 255,
                         255);
            }
            break;
        case led_start_mode_colour:
            for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
                start_rgb_l_leds[i] = con_state.led_solid_l[0];
                start_rgb_r_leds[i] = con_state.led_solid_r[0];
            }
            break;
        case led_start_mode_hid:
#ifdef POWER_SAVE_BRIGHTNESS
            for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
                start_rgb_l_leds[i].setRGB(hid_led_data.leds.rgb[4].r / 4,
                                           hid_led_data.leds.rgb[4].g / 4,
                                           hid_led_data.leds.rgb[4].b / 4);
                start_rgb_r_leds[i].setRGB(hid_led_data.leds.rgb[4].r / 4,
                                           hid_led_data.leds.rgb[4].g / 4,
                                           hid_led_data.leds.rgb[4].b / 4);
            }
#else
            for (uint8_t i = 0; i < PinConf::start_rgb_count; i += 2) {
                memcpy(start_rgb_l_leds[i].raw, hid_led_data.leds.rgb[4].raw, 3);
                memcpy(start_rgb_r_leds[i].raw, hid_led_data.leds.rgb[4].raw, 3);
            }
#endif
            break;
        default:
            break;
    }
}
void do_laser_leds() {
    static int8_t l_shoot_start = 0;
    static int8_t r_shoot_start = 0;
    static int8_t l_shoot_wing_upper = 0;
    static int8_t r_shoot_wing_upper = 0;

    static int8_t l_fill_wing_lower = 0;
    static int8_t l_fill_wing_upper = 0;
    static int8_t r_fill_wing_lower = 0;
    static int8_t r_fill_wing_upper = 0;
    static int8_t fill_start = 0;

    if (vol_x_dir_led > 0 && !l_shoot_start) l_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_x_dir_led < 0 && !l_shoot_start) l_shoot_start = -PinConf::start_rgb_count * 2;
    if (vol_y_dir_led > 0 && !r_shoot_start) r_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_y_dir_led < 0 && !r_shoot_start) r_shoot_start = -PinConf::start_rgb_count * 2;

    if (vol_x_dir_led > 0 && !l_shoot_wing_upper) l_shoot_wing_upper = -PinConf::wing_rgb_upper;
    if (vol_x_dir_led < 0 && !l_shoot_wing_upper) l_shoot_wing_upper = PinConf::wing_rgb_upper;
    if (vol_y_dir_led > 0 && !r_shoot_wing_upper) r_shoot_wing_upper = PinConf::wing_rgb_upper;
    if (vol_y_dir_led < 0 && !r_shoot_wing_upper) r_shoot_wing_upper = -PinConf::wing_rgb_upper;

    if (con_state.reactive_buttons) {
        if (buttons & PinConf::FX_L && !l_fill_wing_lower)
            l_fill_wing_lower = PinConf::wing_rgb_lower;
        if (buttons & PinConf::FX_R && !r_fill_wing_lower)
            r_fill_wing_lower = PinConf::wing_rgb_lower;

        if (buttons & (PinConf::BT_A | PinConf::BT_B) && !l_fill_wing_upper)
            l_fill_wing_upper = PinConf::wing_rgb_upper;
        if (buttons & (PinConf::BT_C | PinConf::BT_D) && !r_fill_wing_upper)
            r_fill_wing_upper = PinConf::wing_rgb_upper;

        if (buttons & PinConf::START && !fill_start) fill_start = PinConf::start_rgb_count;
    }

    vol_x_dir_led = vol_y_dir_led = 0;

#define zone(side, i)                                                    \
    (con_state.led_mode.lasers == led_laser_mode_white ? CHSV(0, 0, 255) \
                                                       : (con_state.led_solid_##side)[i])

    switch (con_state.led_mode.lasers) {
        case led_laser_mode_white:
        case led_laser_mode_colour:
            render_led_shoot_start(l_shoot_start, zone(l, 0));
            render_led_shoot_start(r_shoot_start, zone(r, 0));

            render_led_shoot_wing<PinConf::wing_rgb_upper>(l_shoot_wing_upper, zone(l, 1),
                                                           wing_rgb_l_leds.top);
            render_led_shoot_wing<PinConf::wing_rgb_upper>(r_shoot_wing_upper, zone(r, 1),
                                                           wing_rgb_r_leds.top);

            // Button effects
            render_led_fill<PinConf::start_rgb_count, false>(fill_start, zone(l, 0),
                                                             start_rgb_l_leds);
            render_led_fill<PinConf::start_rgb_count, false>(fill_start, zone(r, 0),
                                                             start_rgb_r_leds);

            render_led_fill<PinConf::wing_rgb_upper, true>(l_fill_wing_upper, zone(l, 1),
                                                           wing_rgb_l_leds.top);
            render_led_fill<PinConf::wing_rgb_upper, true>(r_fill_wing_upper, zone(r, 1),
                                                           wing_rgb_r_leds.top);

            render_led_fill<PinConf::wing_rgb_lower, false>(l_fill_wing_lower, zone(l, 2),
                                                            wing_rgb_l_leds.bottom);
            render_led_fill<PinConf::wing_rgb_lower, false>(r_fill_wing_lower, zone(r, 2),
                                                            wing_rgb_r_leds.bottom);
            break;
        default:
            break;
    }
#undef zone

    toward_zero(&l_shoot_start);
    toward_zero(&r_shoot_start);
    toward_zero(&l_shoot_wing_upper);
    toward_zero(&r_shoot_wing_upper);

    if (fill_start > 0) {
        if ((--fill_start) == 0) fill_start = -PinConf::wing_rgb_upper;
    } else if (fill_start < 0)
        fill_start++;

    if (l_fill_wing_upper > 0) {
        if ((--l_fill_wing_upper) == 0) l_fill_wing_upper = -PinConf::wing_rgb_upper;
    } else if (l_fill_wing_upper < 0)
        l_fill_wing_upper++;

    if (r_fill_wing_upper > 0) {
        if ((--r_fill_wing_upper) == 0) r_fill_wing_upper = -PinConf::wing_rgb_upper;
    } else if (r_fill_wing_upper < 0)
        r_fill_wing_upper++;

    // We're stepping two at a time, so will overshoot sometimes
    constexpr int8_t wing_rgb_zero = (PinConf::wing_rgb_lower % 2 == 0) ? 0 : -1;

    if (l_fill_wing_lower > 0) {
        l_fill_wing_lower -= 2;
        if (l_fill_wing_lower == wing_rgb_zero) l_fill_wing_lower = -PinConf::wing_rgb_lower;
    } else if (l_fill_wing_lower < 0) {
        l_fill_wing_lower += 2;
        if (l_fill_wing_lower > 0) l_fill_wing_lower = 0;
    }

    if (r_fill_wing_lower > 0) {
        r_fill_wing_lower -= 2;
        if (r_fill_wing_lower == wing_rgb_zero) r_fill_wing_lower = -PinConf::wing_rgb_lower;
    } else if (r_fill_wing_lower < 0) {
        r_fill_wing_lower += 2;
        if (r_fill_wing_lower > 0) r_fill_wing_lower = 0;
    }
}

// Writes the animation values for LEDs into button_leds
// The controller UI may override these values before we render them to the pins
void do_button_leds() {
    button_leds = 0;

    led_button_mode_t mode = con_state.led_mode.buttons;
    if (AutoHidOn()) mode = led_button_mode_mixed;

    switch (mode) {
        case led_button_mode_live:
            for (uint8_t i = 0; i < len(hid_led_data.leds.singles); i++) {
                button_leds |= buttons & (1 << i);
            }
            break;
        case led_button_mode_hid:
            for (uint8_t i = 0; i < len(hid_led_data.leds.singles); i++) {
                if (hid_led_data.leds.singles[i].brightness) button_leds |= 1 << i;
            }
            break;
        case led_button_mode_mixed:
            for (uint8_t i = 0; i < len(hid_led_data.leds.singles); i++) {
                if (hid_led_data.leds.singles[i].brightness)
                    button_leds |= 1 << i;
                else
                    button_leds |= buttons & (1 << i);
            }
            break;
        default:
            break;
    }
}

// Output the final value of button_leds to pins
void write_button_leds() {
    for (uint8_t i = 0; i < len(hid_led_data.leds.singles); i++) {
        digitalWrite(PinConf::buttons[i].led, button_leds & (1 << i));
    }
}

void do_leds() {
    blank_led();
    do_wing_upper_leds();
    do_wing_lower_leds();
    do_start_leds();
    do_laser_leds();

    static bool leds_were_off = false;

    // If LEDs aren't being used, make sure they're blank, then stop writing to them.
    if (LEDs_are_off()) {
        if (!leds_were_off) {
            leds_were_off = true;

            blank_led();
            FastLED.show();
        }
    } else {
        leds_were_off = false;

        FastLED.setBrightness(led_brightness);
        // ! NOTE: Yuan packed so many LEDs into this bad boy (90!) that this call is unreasonably
        // ! espensive.
        FastLED.show();
    }

    led_animation_frame += 3;

    do_button_leds();
}
