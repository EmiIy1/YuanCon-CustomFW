#include <Arduino.h>

#include "HID-Settings.h"
#include "HID.h"

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

void light_update(SingleLED* single_leds, RGBLed* rgb_leds);

#define NUMBER_OF_LIGHTS (NUMBER_OF_SINGLE + NUMBER_OF_RGB * 3)
#if NUMBER_OF_LIGHTS > 63
#error You must have less than 64 lights
#endif

typedef union {
    struct {
        SingleLED singles[NUMBER_OF_SINGLE];
        RGBLed rgb[NUMBER_OF_RGB];
    } leds;
    uint8_t raw[NUMBER_OF_LIGHTS];
} led_data_t;
extern led_data_t hid_led_data;

class HIDLED_ : public PluggableUSBModule {
    EPTYPE_DESCRIPTOR_SIZE epType[1];

   public:
    HIDLED_(void);
    void begin(void);
    int getInterface(uint8_t* interfaceCount);
    int getDescriptor(USBSetup& setup);
    bool setup(USBSetup& setup);
};

extern HIDLED_ HIDLeds;
