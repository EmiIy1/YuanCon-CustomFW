constexpr uint8_t HID_REPORTID_MINI_GAMEPAD = 1;
// USC is treating the konami PID/VID pair as an SDVX Pico, and asserting it's report ID 2.
// For now, rather than PRing support into upstream USC, let's just make a compatible mode.
constexpr uint8_t HID_REPORTID_PICO_LEDS = 2;
constexpr uint8_t HID_REPORTID_LEDS = 3;
constexpr uint8_t HID_REPORTID_MINI_MOUSE = 4;
constexpr uint8_t HID_REPORTID_MINI_KEYBOARD = 5;
constexpr uint8_t HID_REPORTID_MINI_CONTROL = 7;

constexpr uint8_t HID_Strings_Base = ISERIAL + 1;
constexpr const char* HID_strings[] = {
    // Single lights
    "BT-A",
    "BT-B",
    "BT-C",
    "BT-D",
    "FX-L",
    "FX-R",
    "Start",
    // RGB lights
    "Wing Left Up",
    NULL,
    NULL,
    "Wing Right Up",
    NULL,
    NULL,
    "Wing Left Low",
    NULL,
    NULL,
    "Wing Right Low",
    NULL,
    NULL,
    "Top LEDs Left",
    NULL,
    NULL,
    "Top LEDs Right",
    NULL,
    NULL,
    // Extra strings
    "USC Lighting",
};
// Start indecies in HID_strings
constexpr uint8_t HID_strings_leds = HID_Strings_Base + 0;
constexpr uint8_t HID_strings_rgb = HID_Strings_Base + 7;
constexpr uint8_t HID_strings_rgb_end = HID_Strings_Base + 25;
constexpr uint8_t HID_strings_pico = HID_Strings_Base + 25;
