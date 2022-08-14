#include "leds.h"

#include "HID_lighting.h"
#include "buttons.h"
#include "pins.h"
#include "vol.h"

uint16_t button_leds;

CRGB wing_rgb_l_leds[PinConf::wing_rgb_count];
CRGB wing_rgb_r_leds[PinConf::wing_rgb_count];
CRGB start_rgb_l_leds[PinConf::start_rgb_count];
CRGB start_rgb_r_leds[PinConf::start_rgb_count];

CHSV led_solid_l = { 0, 255, 255 };
CHSV led_solid_r = { 0, 255, 255 };

led_mode_config_t builtin_modes[] = {
    // Official Yuan modes
    {
        led_laser_mode_white,
        led_start_mode_none,
        led_wing_mode_rainbow,
        led_button_mode_live,
    },
    {
        led_laser_mode_colour,
        led_start_mode_none,
        led_wing_mode_colour,
        led_button_mode_live,
    },
    {
        led_laser_mode_colour,
        led_start_mode_none,
        led_wing_mode_none,
        led_button_mode_live,
    },
    {
        led_laser_mode_none,
        led_start_mode_none,
        led_wing_mode_none,
        led_button_mode_live,
    },
};
led_mode_config_t led_mode = builtin_modes[0];

led_mode_config_t* led_quick_dial[4] = {
    &builtin_modes[0],
    &builtin_modes[1],
    &builtin_modes[2],
    &builtin_modes[3],
};

bool auto_hid = true;

void blank_led() {
    for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
        wing_rgb_l_leds[i] = CRGB(0, 0, 0);
        wing_rgb_r_leds[i] = CRGB(0, 0, 0);
    }
    for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
        start_rgb_l_leds[i] = CRGB(0, 0, 0);
        start_rgb_r_leds[i] = CRGB(0, 0, 0);
    }
}

void do_race(uint8_t* race, uint8_t* dir, CRGB colour) {
    if (*race < PinConf::wing_rgb_count) {
        wing_rgb_l_leds[*race] = colour;
    } else if (*race < PinConf::wing_rgb_count + PinConf::start_rgb_count) {
        start_rgb_l_leds[PinConf::start_rgb_count - (*race - PinConf::wing_rgb_count) - 1] = colour;
    } else if (*race < PinConf::wing_rgb_count + PinConf::start_rgb_count * 2) {
        start_rgb_r_leds[*race - (PinConf::wing_rgb_count + PinConf::start_rgb_count)] = colour;
    } else {
        wing_rgb_r_leds[PinConf::wing_rgb_count -
                        (*race - (PinConf::wing_rgb_count + PinConf::start_rgb_count * 2)) - 1] =
            colour;
    }

    if (*dir == 0) {
        if ((++(*race)) == (PinConf::wing_rgb_count + PinConf::start_rgb_count) * 2) {
            *dir = 1;
            (*race)--;
        }
    } else {
        if (((*race)--) == 0) {
            *dir = 0;
            (*race)++;
        }
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

void render_led_shoot_wing(int8_t shoot, CHSV colour, CRGB* leds) {
    if (shoot > 0) {
        leds[PinConf::wing_rgb_lower + (PinConf::wing_rgb_upper - shoot)] = colour;
    }
    if (shoot < 0) {
        leds[PinConf::wing_rgb_lower - shoot - 1] = colour;
    }
}

void setup_leds() {
    FastLED.addLeds<WS2812, PinConf::wing_rgb_l, GRB>(wing_rgb_l_leds, PinConf::wing_rgb_count);
    FastLED.addLeds<WS2812, PinConf::wing_rgb_r, GRB>(wing_rgb_r_leds, PinConf::wing_rgb_count);
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

void do_hid_leds() {
    // HID RGB is:
    // - Wing left up
    // - Wing right up
    // - Wing left low
    // - Wing right low
    // - Woofer

    if (!hid_dirty) return;
    hid_dirty = false;

#define POWER_SAVE_BRIGHTNESS

    blank_led();

#ifdef POWER_SAVE_BRIGHTNESS
    for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
        if (i < PinConf::wing_rgb_lower) {
            wing_rgb_l_leds[i].setRGB(hid_led_data.leds.rgb[2].r / 4,
                                      hid_led_data.leds.rgb[2].g / 4,
                                      hid_led_data.leds.rgb[2].b / 4);
            wing_rgb_r_leds[i].setRGB(hid_led_data.leds.rgb[3].r / 4,
                                      hid_led_data.leds.rgb[3].g / 4,
                                      hid_led_data.leds.rgb[3].b / 4);
        } else {
            wing_rgb_l_leds[i].setRGB(hid_led_data.leds.rgb[0].r / 4,
                                      hid_led_data.leds.rgb[0].g / 4,
                                      hid_led_data.leds.rgb[0].b / 4);
            wing_rgb_r_leds[i].setRGB(hid_led_data.leds.rgb[1].r / 4,
                                      hid_led_data.leds.rgb[1].g / 4,
                                      hid_led_data.leds.rgb[1].b / 4);
        }
    }

    for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
        start_rgb_l_leds[i].setRGB(hid_led_data.leds.rgb[4].r / 4, hid_led_data.leds.rgb[4].g / 4,
                                   hid_led_data.leds.rgb[4].b / 4);
        start_rgb_r_leds[i].setRGB(hid_led_data.leds.rgb[4].r / 4, hid_led_data.leds.rgb[4].g / 4,
                                   hid_led_data.leds.rgb[4].b / 4);
    }
#else
    for (uint8_t i = 0; i < PinConf::wing_rgb_count; i += 2) {
        if (i < PinConf::wing_rgb_lower) {
            memcpy(wing_rgb_l_leds[i].raw, hid_led_data.leds.rgb[2].raw, 3);
            memcpy(wing_rgb_r_leds[i].raw, hid_led_data.leds.rgb[3].raw, 3);
        } else {
            memcpy(wing_rgb_l_leds[i].raw, hid_led_data.leds.rgb[0].raw, 3);
            memcpy(wing_rgb_r_leds[i].raw, hid_led_data.leds.rgb[1].raw, 3);
        }
    }

    for (uint8_t i = 0; i < PinConf::start_rgb_count; i += 2) {
        memcpy(start_rgb_l_leds[i].raw, hid_led_data.leds.rgb[4].raw, 3);
        memcpy(start_rgb_r_leds[i].raw, hid_led_data.leds.rgb[4].raw, 3);
    }
#endif
    FastLED.show();
}

uint8_t led_animation_frame = 0;
void do_wing_leds() {
    if (LED_has_colour() && buttons & KeyMap::led_colour) {
        for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
            wing_rgb_l_leds[i] = led_solid_l;
            wing_rgb_r_leds[i] = led_solid_r;
        }
        return;
    }

    led_wing_mode_t mode = led_mode.wing;
    if (auto_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
        mode = led_wing_mode_hid;
    }

    switch (mode) {
        case led_wing_mode_hid:
#ifdef POWER_SAVE_BRIGHTNESS
            for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
                if (i < PinConf::wing_rgb_lower) {
                    wing_rgb_l_leds[i].setRGB(hid_led_data.leds.rgb[2].r / 4,
                                              hid_led_data.leds.rgb[2].g / 4,
                                              hid_led_data.leds.rgb[2].b / 4);
                    wing_rgb_r_leds[i].setRGB(hid_led_data.leds.rgb[3].r / 4,
                                              hid_led_data.leds.rgb[3].g / 4,
                                              hid_led_data.leds.rgb[3].b / 4);
                } else {
                    wing_rgb_l_leds[i].setRGB(hid_led_data.leds.rgb[0].r / 4,
                                              hid_led_data.leds.rgb[0].g / 4,
                                              hid_led_data.leds.rgb[0].b / 4);
                    wing_rgb_r_leds[i].setRGB(hid_led_data.leds.rgb[1].r / 4,
                                              hid_led_data.leds.rgb[1].g / 4,
                                              hid_led_data.leds.rgb[1].b / 4);
                }
            }
#else
            for (uint8_t i = 0; i < PinConf::wing_rgb_count; i += 2) {
                if (i < PinConf::wing_rgb_lower) {
                    memcpy(wing_rgb_l_leds[i].raw, hid_led_data.leds.rgb[2].raw, 3);
                    memcpy(wing_rgb_r_leds[i].raw, hid_led_data.leds.rgb[3].raw, 3);
                } else {
                    memcpy(wing_rgb_l_leds[i].raw, hid_led_data.leds.rgb[0].raw, 3);
                    memcpy(wing_rgb_r_leds[i].raw, hid_led_data.leds.rgb[1].raw, 3);
                }
            }
#endif
            break;
        case led_wing_mode_rainbow:
            for (uint8_t i = 0; i < PinConf::wing_rgb_lower; i++) {
                wing_rgb_l_leds[PinConf::wing_rgb_lower - i] = CHSV(
                    led_animation_frame + i * (256 / ((float)(PinConf::wing_rgb_lower))), 255, 255);
                wing_rgb_r_leds[PinConf::wing_rgb_lower - i] = CHSV(
                    led_animation_frame + i * (256 / ((float)(PinConf::wing_rgb_lower))), 255, 255);
            }
            for (uint8_t i = 0; i < PinConf::wing_rgb_upper; i++) {
                wing_rgb_l_leds[PinConf::wing_rgb_lower + i] = CHSV(
                    led_animation_frame + i * (256 / ((float)PinConf::wing_rgb_upper)), 255, 255);
                wing_rgb_r_leds[PinConf::wing_rgb_lower + i] = CHSV(
                    led_animation_frame + i * (256 / ((float)PinConf::wing_rgb_upper)), 255, 255);
            }
            break;
        case led_wing_mode_colour:
            for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
                wing_rgb_l_leds[i] = led_solid_l;
                wing_rgb_r_leds[i] = led_solid_r;
            }
            break;
        default:
            break;
    }
}

void do_start_leds() {
    led_start_mode_t mode = led_mode.start;
    if (auto_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
        mode = led_start_mode_hid;
    }

    switch (mode) {
        case led_start_mode_rainbow:
            for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
                start_rgb_l_leds[i] =
                    CHSV(led_animation_frame + i * (256 / ((float)(PinConf::start_rgb_count))), 255,
                         255);
                start_rgb_r_leds[i] =
                    CHSV(led_animation_frame + i * (256 / ((float)(PinConf::start_rgb_count))), 255,
                         255);
            }
            break;
        case led_start_mode_colour:
            for (uint8_t i = 0; i < PinConf::start_rgb_count; i++) {
                start_rgb_l_leds[i] = led_solid_l;
                start_rgb_r_leds[i] = led_solid_r;
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
    static int8_t l_shoot_wing = 0;
    static int8_t r_shoot_wing = 0;

    if (vol_x_dir_led > 0 && !l_shoot_start) l_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_x_dir_led < 0 && !l_shoot_start) l_shoot_start = -PinConf::start_rgb_count * 2;
    if (vol_y_dir_led > 0 && !r_shoot_start) r_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_y_dir_led < 0 && !r_shoot_start) r_shoot_start = -PinConf::start_rgb_count * 2;

    if (vol_x_dir_led > 0 && !l_shoot_wing) l_shoot_wing = -PinConf::wing_rgb_upper;
    if (vol_x_dir_led < 0 && !l_shoot_wing) l_shoot_wing = PinConf::wing_rgb_upper;
    if (vol_y_dir_led > 0 && !r_shoot_wing) r_shoot_wing = PinConf::wing_rgb_upper;
    if (vol_y_dir_led < 0 && !r_shoot_wing) r_shoot_wing = -PinConf::wing_rgb_upper;

    vol_x_dir_led = vol_y_dir_led = 0;

    switch (led_mode.lasers) {
        case led_laser_mode_white:
            render_led_shoot_start(l_shoot_start, CHSV(0, 0, 255));
            render_led_shoot_start(r_shoot_start, CHSV(0, 0, 255));
            render_led_shoot_wing(l_shoot_wing, CHSV(0, 0, 255), wing_rgb_l_leds);
            render_led_shoot_wing(r_shoot_wing, CHSV(0, 0, 255), wing_rgb_r_leds);
            break;
        case led_laser_mode_colour:
            render_led_shoot_start(l_shoot_start, led_solid_l);
            render_led_shoot_start(r_shoot_start, led_solid_r);
            render_led_shoot_wing(l_shoot_wing, led_solid_l, wing_rgb_l_leds);
            render_led_shoot_wing(r_shoot_wing, led_solid_r, wing_rgb_r_leds);
            break;
        default:
            break;
    }

    toward_zero(&l_shoot_start);
    toward_zero(&r_shoot_start);
    toward_zero(&l_shoot_wing);
    toward_zero(&r_shoot_wing);
}

// Writes the animation values for LEDs into button_leds
// The controller UI may override these values before we render them to the pins
void do_button_leds() {
    button_leds = 0;

    led_button_mode_t mode = led_mode.buttons;
    if (auto_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
        mode = led_button_mode_mixed;
    }

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
    static uint8_t tick = 0;

    if ((++tick) == 10) {
        tick = 0;

        blank_led();
        do_wing_leds();
        do_start_leds();
        do_laser_leds();
        // ! NOTE: Yuan packed so many LEDs into this bad boy (90!) that this call is unreasonably
        // ! espensive.
        FastLED.show();

        led_animation_frame += 3;
    }
    do_button_leds();
}
