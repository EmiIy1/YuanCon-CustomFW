#include "Lighting.h"

#include <hid_def.h>

#include "Custom-HID.h"

HID_LedsReport_Data_t hid_led_data = { 0 };
unsigned long last_hid = 0;
bool hid_need_ack = false;

#define _HID_LEDs(x, n)                                                                          \
    HID_USAGE_PAGE(ORDINAL), HID_USAGE2(1, n + 1), HID_COLLECTION(LOGICAL), HID_USAGE_PAGE(LED), \
        HID_USAGE(GENERIC_INDICATOR), HID_STRING_INDEX(1, HID_strings_leds + x),                 \
        HID_REPORT_SIZE(8), HID_REPORT_COUNT(n), HID_OUTPUT(DATA, VARIABLE, ABSOLUTE),           \
        HID_END_COLLECTION(LOGICAL)

#define HID_SingleLED(x) _HID_LEDs(x, 1)
// TODO: Figure out why a report count of 3 wasn't working in spice
#define HID_ThreeLEDs(x) HID_SingleLED(x), HID_SingleLED(x + 1), HID_SingleLED(x + 2)

static const uint8_t _hidReportLEDs[] PROGMEM = {
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(UNDEFINED),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_LEDS),
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(2, 255),

    // Buttons
    HID_SingleLED(0),
    HID_SingleLED(1),
    HID_SingleLED(2),
    HID_SingleLED(3),
    HID_SingleLED(4),
    HID_SingleLED(5),
    HID_SingleLED(6),

    // Lighting regions
    HID_ThreeLEDs(7),
    HID_ThreeLEDs(10),
    HID_ThreeLEDs(13),
    HID_ThreeLEDs(16),
    HID_ThreeLEDs(19),
    HID_ThreeLEDs(22),

    // BTools needs at least 1 input to work properly
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, 1),
    HID_INPUT(CONSTANT, VARIABLE, ABSOLUTE),

    HID_END_COLLECTION(APPLICATION),
};

// Spoofed SDVX Pico for USC
static const uint8_t _hidReportLEDsPico[] PROGMEM = {
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(UNDEFINED),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_PICO_LEDS),

    //
    HID_REPORT_COUNT(PicoNoLED),
    HID_REPORT_SIZE(8),
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(2, 255),
    HID_USAGE_PAGE(LED),
    HID_USAGE(GENERIC_INDICATOR),
    HID_STRING_INDEX(1, HID_strings_pico),
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, PicoNoLED),
    HID_OUTPUT(DATA, VARIABLE, ABSOLUTE),

    // BTools needs at least 1 input to work properly
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, 1),
    HID_INPUT(CONSTANT, VARIABLE, ABSOLUTE),

    HID_END_COLLECTION(APPLICATION),
};

bool leds_callback(uint16_t length) {
    if (length != sizeof hid_led_data) return false;
    USBDevice.recvControl(&hid_led_data, sizeof hid_led_data);
    last_hid = millis();
    hid_need_ack = true;

    return true;
}

bool pico_callback(uint16_t length) {
    HID_PicoLedsReport_Data_t report;
    if (length != sizeof report) return false;
    USBDevice.recvControl(&report, sizeof report + 1);
    memcpy(hid_led_data.leds.singles, report.leds.singles, PicoNoSingle);
    hid_led_data.leds.rgb[0] = report.leds.rgb[0];
    hid_led_data.leds.rgb[1] = report.leds.rgb[1];
    hid_led_data.leds.rgb[2] = report.leds.rgb[0];
    hid_led_data.leds.rgb[3] = report.leds.rgb[1];
    memset(hid_led_data.leds.rgb[4].raw, 0, 3);
    memset(hid_led_data.leds.rgb[5].raw, 0, 3);

    last_hid = millis();
    hid_need_ack = true;

    return true;
}

HIDLeds_::HIDLeds_(void) {
    static HIDSubDescriptor node(_hidReportLEDs, sizeof(_hidReportLEDs));
    static HIDCallback callback = { &leds_callback, HID_REPORTID_LEDS, NULL };

    CustomHID().AppendDescriptor(&node, HID_INTERFACE_LIGHTS);
    CustomHID().AppendCallback(&callback, HID_INTERFACE_LIGHTS);

    static HIDSubDescriptor nodePico(_hidReportLEDsPico, sizeof(_hidReportLEDsPico));
    static HIDCallback callbackPico = { &pico_callback, HID_REPORTID_PICO_LEDS, NULL };
    CustomHID().AppendDescriptor(&nodePico, HID_INTERFACE_PICO_LIGHTS);
    CustomHID().AppendCallback(&callbackPico, HID_INTERFACE_PICO_LIGHTS);
}
void HIDLeds_::begin(void) { return; }

HIDLeds_ HIDLeds;
