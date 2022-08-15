#include "leds.h"

#include "HID/Lighting.h"
#include "buttons.h"
#include "persist.h"
#include "pins.h"
#include "vol.h"

uint16_t button_leds;

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
    if (con_state.auto_hid && last_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
        mode = led_wing_mode_hid;
    }

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
    if (con_state.auto_hid && last_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
        mode = led_wing_mode_hid;
    }

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
    if (con_state.auto_hid && last_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
        mode = led_start_mode_hid;
    }

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
    static int8_t l_shoot_wing_lower = 0;
    static int8_t r_shoot_wing_lower = 0;
    static bool invert_start_shoot_l = false;
    static bool invert_start_shoot_r = false;

    if (vol_x_dir_led > 0 && !l_shoot_start) l_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_x_dir_led < 0 && !l_shoot_start) l_shoot_start = -PinConf::start_rgb_count * 2;
    if (vol_y_dir_led > 0 && !r_shoot_start) r_shoot_start = PinConf::start_rgb_count * 2;
    if (vol_y_dir_led < 0 && !r_shoot_start) r_shoot_start = -PinConf::start_rgb_count * 2;

    if (vol_x_dir_led > 0 && !l_shoot_wing_upper) l_shoot_wing_upper = -PinConf::wing_rgb_upper;
    if (vol_x_dir_led < 0 && !l_shoot_wing_upper) l_shoot_wing_upper = PinConf::wing_rgb_upper;
    if (vol_y_dir_led > 0 && !r_shoot_wing_upper) r_shoot_wing_upper = PinConf::wing_rgb_upper;
    if (vol_y_dir_led < 0 && !r_shoot_wing_upper) r_shoot_wing_upper = -PinConf::wing_rgb_upper;

    if (con_state.reactive_buttons) {
        if (buttons & PinConf::FX_L && !l_shoot_wing_lower)
            l_shoot_wing_lower = PinConf::wing_rgb_lower;
        if (buttons & PinConf::FX_R && !r_shoot_wing_lower)
            r_shoot_wing_lower = PinConf::wing_rgb_lower;
        if (buttons & (PinConf::BT_A | PinConf::BT_B) && !l_shoot_wing_upper)
            l_shoot_wing_upper = -PinConf::wing_rgb_upper;
        if (buttons & (PinConf::BT_C | PinConf::BT_D) && !r_shoot_wing_upper)
            r_shoot_wing_upper = -PinConf::wing_rgb_upper;

        if (buttons & PinConf::START && !l_shoot_start) {
            r_shoot_start = -PinConf::start_rgb_count;
            l_shoot_start = PinConf::start_rgb_count;
            // We're hijacking the knob animation mid-animation, so need inverted colours
            invert_start_shoot_l = invert_start_shoot_r = true;
        }
    }

    vol_x_dir_led = vol_y_dir_led = 0;

    switch (con_state.led_mode.lasers) {
        case led_laser_mode_white:
            render_led_shoot_start(l_shoot_start, CHSV(0, 0, 255));
            render_led_shoot_start(r_shoot_start, CHSV(0, 0, 255));
            render_led_shoot_wing<PinConf::wing_rgb_upper>(l_shoot_wing_upper, CHSV(0, 0, 255),
                                                           wing_rgb_l_leds.top);
            render_led_shoot_wing<PinConf::wing_rgb_upper>(r_shoot_wing_upper, CHSV(0, 0, 255),
                                                           wing_rgb_r_leds.top);
            render_led_shoot_wing<PinConf::wing_rgb_lower>(l_shoot_wing_lower, CHSV(255, 0, 255),
                                                           wing_rgb_l_leds.bottom);
            render_led_shoot_wing<PinConf::wing_rgb_lower>(r_shoot_wing_lower, CHSV(255, 0, 255),
                                                           wing_rgb_r_leds.bottom);
            break;
        case led_laser_mode_colour:
            render_led_shoot_start(l_shoot_start, invert_start_shoot_l ? con_state.led_solid_r[0]
                                                                       : con_state.led_solid_l[0]);
            render_led_shoot_start(r_shoot_start, invert_start_shoot_r ? con_state.led_solid_l[0]
                                                                       : con_state.led_solid_r[0]);
            render_led_shoot_wing<PinConf::wing_rgb_upper>(
                l_shoot_wing_upper, con_state.led_solid_l[1], wing_rgb_l_leds.top);
            render_led_shoot_wing<PinConf::wing_rgb_upper>(
                r_shoot_wing_upper, con_state.led_solid_r[1], wing_rgb_r_leds.top);
            render_led_shoot_wing<PinConf::wing_rgb_lower>(
                l_shoot_wing_lower, con_state.led_solid_l[2], wing_rgb_l_leds.bottom);
            render_led_shoot_wing<PinConf::wing_rgb_lower>(
                r_shoot_wing_lower, con_state.led_solid_r[2], wing_rgb_r_leds.bottom);
            break;
        default:
            break;
    }

    toward_zero(&l_shoot_start);
    toward_zero(&r_shoot_start);
    toward_zero(&l_shoot_wing_upper);
    toward_zero(&r_shoot_wing_upper);
    toward_zero(&l_shoot_wing_lower);
    toward_zero(&r_shoot_wing_lower);
    if (l_shoot_start == 0) invert_start_shoot_l = false;
    if (r_shoot_start == 0) invert_start_shoot_r = false;
}

// Writes the animation values for LEDs into button_leds
// The controller UI may override these values before we render them to the pins
void do_button_leds() {
    button_leds = 0;

    led_button_mode_t mode = con_state.led_mode.buttons;
    if (con_state.auto_hid && last_hid && millis() - last_hid < AUTO_HID_TIMEOUT) {
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
    blank_led();
    do_wing_upper_leds();
    do_wing_lower_leds();
    do_start_leds();
    do_laser_leds();
    // ! NOTE: Yuan packed so many LEDs into this bad boy (90!) that this call is unreasonably
    // ! espensive.
    FastLED.show();

    led_animation_frame += 3;

    do_button_leds();
}
