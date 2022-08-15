#pragma once

#include <HID.h>
#include <vendor.h>

typedef struct {
    uint8_t brightness;
} SingleLED;

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    uint8_t raw[3];
} RGBLed;

constexpr uint8_t NUMBER_OF_SINGLE = 7;
constexpr uint8_t NUMBER_OF_RGB = 5;  // four wing + woofer

extern bool hid_dirty;
extern unsigned long last_hid;
constexpr unsigned long AUTO_HID_TIMEOUT = 3000;  // 3 seconds

void light_update(SingleLED* single_leds, RGBLed* rgb_leds);

#define NUMBER_OF_LIGHTS (NUMBER_OF_SINGLE + NUMBER_OF_RGB * 3)
#if NUMBER_OF_LIGHTS > 63
#error You must have less than 64 lights
#endif

typedef struct {
    uint8_t report_id;
    union {
        struct {
            SingleLED singles[NUMBER_OF_SINGLE];
            RGBLed rgb[NUMBER_OF_RGB];
        } leds;
        uint8_t raw[NUMBER_OF_LIGHTS];
    };
} HID_LedsReport_Data_t;
extern HID_LedsReport_Data_t hid_led_data;

class HIDLeds_ {
   public:
    HIDLeds_(void);
    void begin(void);
};

extern HIDLeds_ HIDLeds;
