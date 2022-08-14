#include <Arduino.h>

#define len(x) (sizeof(x) / sizeof(x)[0])

namespace PinConf {

typedef struct {
    pin_size_t btn;
    pin_size_t led;
} button_pins_t;

constexpr button_pins_t bt_a = { 15, 16 };
constexpr button_pins_t bt_b = { 17, 18 };
constexpr button_pins_t bt_c = { 40, 9 };
constexpr button_pins_t bt_d = { 23, 24 };

constexpr button_pins_t fx_l = { 14, 19 };
constexpr button_pins_t fx_r = { 10, 12 };

constexpr button_pins_t start = { 2, 5 };

constexpr button_pins_t ex_1 = { 30, 31 };
constexpr button_pins_t ex_2 = { 32, 33 };
constexpr button_pins_t ex_3 = { 6, 7 };

constexpr uint16_t BT_A = 1;
constexpr uint16_t BT_B = 2;
constexpr uint16_t BT_C = 4;
constexpr uint16_t BT_D = 8;
constexpr uint16_t FX_L = 16;
constexpr uint16_t FX_R = 32;
constexpr uint16_t START = 64;
constexpr uint16_t EX_1 = 128;
constexpr uint16_t EX_2 = 256;
constexpr uint16_t EX_3 = 512;

constexpr button_pins_t buttons[] = { bt_a, bt_b, bt_c, bt_d, fx_l, fx_r, start, ex_1, ex_2, ex_3 };
constexpr char keymap[] = { 'd', 'f', 'j', 'k', 'v', 'n', 't', 'q', 'a', 'z' };
constexpr uint8_t gamepad_map[] = { 1, 2, 3, 4, 5, 6, 9, 10, 8, 7 };

typedef struct {
    pin_size_t g1;
    pin_size_t g2;
} knob_pins_t;

// Need to be interrupt pins, which they are (thanks, Yuan)
constexpr knob_pins_t vol_x = { 0, 1 };
constexpr knob_pins_t vol_y = { 3, 4 };

constexpr pin_size_t wing_rgb_l = 38;
constexpr pin_size_t wing_rgb_r = 11;
// [0:17] = bottom half
// order: -> fx -> edge ->
constexpr uint8_t wing_rgb_lower = 17;
// [17:29] = top half
// order: -> edge -> bt ->
constexpr uint8_t wing_rgb_upper = 12;
constexpr uint8_t wing_rgb_count = wing_rgb_lower + wing_rgb_upper;

constexpr pin_size_t start_rgb_l = 22;
constexpr pin_size_t start_rgb_r = 37;
// order: -> start -> knob ->
constexpr uint8_t start_rgb_count = 16;

}  // namespace PinConf

namespace PinMap {
constexpr uint16_t led_mode = PinConf::EX_1;
constexpr uint16_t led_colour = PinConf::EX_2;

constexpr uint16_t led_reset = PinConf::EX_1 | PinConf::EX_2 | PinConf::BT_B | PinConf::BT_C;
}  // namespace PinMap
