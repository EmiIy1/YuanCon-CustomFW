
#ifndef HID_REPORTID_MINI_MOUSE
#define HID_REPORTID_MINI_MOUSE 10
#endif

#ifndef HID_REPORTID_MINI_KEYBOARD
#define HID_REPORTID_MINI_KEYBOARD 9
#endif

#ifndef HID_REPORTID_MINI_GAMEPAD
#define HID_REPORTID_MINI_GAMEPAD 11
#endif

#ifndef HID_REPORTID_MINI_CONTROL
#define HID_REPORTID_MINI_CONTROL 12
#endif

#ifndef HID_REPORTID_PICO_LEDS
// USC is treating the konami PID/VID pair as an SDVX Pico, and asserting it's report ID 2.
// For now, rather than PRing support into upstream USC, let's just make a compatible mode.
#define HID_REPORTID_PICO_LEDS 2
#endif
#ifndef HID_REPORTID_LEDS
#define HID_REPORTID_LEDS 13
#endif
