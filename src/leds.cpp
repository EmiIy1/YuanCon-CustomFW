#include "leds.h"

#include "buttons.h"
#include "pins.h"
#include "vol.h"

CRGB wing_rgb_l_leds[PinConf::wing_rgb_count];
CRGB wing_rgb_r_leds[PinConf::wing_rgb_count];
CRGB start_rgb_l_leds[PinConf::start_rgb_count];
CRGB start_rgb_r_leds[PinConf::start_rgb_count];

led_mode_t led_mode = led_mode_rainbow;
CHSV led_solid_l = { 0, 255, 255 };
CHSV led_solid_r = { 0, 255, 255 };

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

void render_led_shoot_start(int8_t* shoot, CHSV colour) {
    if ((*shoot) > 0) {
        if ((*shoot) > PinConf::start_rgb_count)
            start_rgb_l_leds[(*shoot) - PinConf::start_rgb_count - 1] = colour;
        else
            start_rgb_r_leds[PinConf::start_rgb_count - (*shoot)] = colour;

        (*shoot)--;
    }
    if ((*shoot) < 0) {
        if ((*shoot) < -PinConf::start_rgb_count)
            start_rgb_r_leds[-(*shoot) - PinConf::start_rgb_count] = colour;
        else
            start_rgb_l_leds[PinConf::start_rgb_count + (*shoot)] = colour;
        (*shoot)++;
    }
}

void render_led_shoot_wing(int8_t* shoot, CHSV colour, CRGB* leds) {
    if ((*shoot) > 0) {
        leds[PinConf::wing_rgb_lower +
             ((PinConf::wing_rgb_count - PinConf::wing_rgb_lower) - (*shoot))] = colour;
        (*shoot)--;
    }
    if ((*shoot) < 0) {
        leds[PinConf::wing_rgb_lower - (*shoot) - 1] = colour;
        (*shoot)++;
    }
}

void do_led_animation() {
    blank_led();

    static uint8_t anim_cycle;
    static int8_t l_shoot_start = 0;
    static int8_t r_shoot_start = 0;
    static int8_t l_shoot_wing = 0;
    static int8_t r_shoot_wing = 0;

    if (led_mode == led_mode_rainbow) {
        for (uint8_t i = 0; i < 18; i++) {
            wing_rgb_l_leds[17 - i] = CHSV(anim_cycle + i * (256 / 18.0), 255, 255);
            wing_rgb_r_leds[17 - i] = CHSV(anim_cycle + i * (256 / 18.0), 255, 255);
        }
        for (uint8_t i = 0; i < 11; i++) {
            wing_rgb_l_leds[18 + i] = CHSV(anim_cycle + i * (256 / 11.0), 255, 255);
            wing_rgb_r_leds[18 + i] = CHSV(anim_cycle + i * (256 / 11.0), 255, 255);
        }
    } else if (led_mode == led_mode_solid ||
               (led_mode == led_mode_lasers && buttons & PinConf::EX_1)) {
        for (uint8_t i = 0; i < PinConf::wing_rgb_count; i++) {
            wing_rgb_l_leds[i] = led_solid_l;
            wing_rgb_r_leds[i] = led_solid_r;
        }
    }

    if (vol_x_dir_led > 0 && !l_shoot_start) l_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_x_dir_led < 0 && !l_shoot_start) l_shoot_start = -PinConf::start_rgb_count * 2;
    if (vol_y_dir_led > 0 && !r_shoot_start) r_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_y_dir_led < 0 && !r_shoot_start) r_shoot_start = -PinConf::start_rgb_count * 2;

    if (vol_x_dir_led > 0 && !l_shoot_wing)
        l_shoot_wing = -(PinConf::wing_rgb_count - PinConf::wing_rgb_lower);
    if (vol_x_dir_led < 0 && !l_shoot_wing)
        l_shoot_wing = (PinConf::wing_rgb_count - PinConf::wing_rgb_lower);
    if (vol_y_dir_led > 0 && !r_shoot_wing)
        r_shoot_wing = (PinConf::wing_rgb_count - PinConf::wing_rgb_lower);
    if (vol_y_dir_led < 0 && !r_shoot_wing)
        r_shoot_wing = -(PinConf::wing_rgb_count - PinConf::wing_rgb_lower);

    vol_x_dir_led = vol_y_dir_led = 0;

    if (led_mode != led_mode_none) {
        render_led_shoot_start(&l_shoot_start,
                               led_mode == led_mode_rainbow ? CHSV(0, 0, 255) : led_solid_l);
        render_led_shoot_start(&r_shoot_start,
                               led_mode == led_mode_rainbow ? CHSV(0, 0, 255) : led_solid_r);

        render_led_shoot_wing(&l_shoot_wing,
                              led_mode == led_mode_rainbow ? CHSV(0, 0, 255) : led_solid_l,
                              wing_rgb_l_leds);
        render_led_shoot_wing(&r_shoot_wing,
                              led_mode == led_mode_rainbow ? CHSV(0, 0, 255) : led_solid_r,
                              wing_rgb_r_leds);
    }

    anim_cycle += 3;

    // static uint8_t race[5] = { 0, 35, 61, 66, 28 };
    // static uint8_t dir[5] = { 0, 0, 0, 1, 1 };
    // do_race(&race[0], &dir[0], CRGB(0, 0, 255));
    // do_race(&race[1], &dir[1], CRGB(255, 0, 0));
    // do_race(&race[2], &dir[2], CRGB(0, 255, 0));
    // do_race(&race[3], &dir[3], CRGB(0, 255, 255));
    // do_race(&race[4], &dir[4], CRGB(255, 0, 255));

    FastLED.show();
}

void setup_leds() {
    FastLED.addLeds<WS2812, PinConf::wing_rgb_l, GRB>(wing_rgb_l_leds, PinConf::wing_rgb_count);
    FastLED.addLeds<WS2812, PinConf::wing_rgb_r, GRB>(wing_rgb_r_leds, PinConf::wing_rgb_count);
    FastLED.addLeds<WS2812, PinConf::start_rgb_l, GRB>(start_rgb_l_leds, PinConf::start_rgb_count);
    FastLED.addLeds<WS2812, PinConf::start_rgb_r, GRB>(start_rgb_r_leds, PinConf::start_rgb_count);
    blank_led();
    FastLED.show();
}
