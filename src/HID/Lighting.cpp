#include "Lighting.h"

#include <hid_def.h>

#include "Custom-HID.h"
#include "IDs.h"

HID_LedsReport_Data_t hid_led_data;
bool hid_dirty = false;
unsigned long last_hid = 0;

static const uint8_t _hidReportLEDs[] PROGMEM = {
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(UNDEFINED),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_LEDS),

    HID_REPORT_COUNT(NUMBER_OF_LIGHTS),
    HID_REPORT_SIZE(8),
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(2, 255),
    HID_USAGE_PAGE(ORDINAL),
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, NUMBER_OF_LIGHTS),
    HID_OUTPUT(DATA, VARIABLE, ABSOLUTE),

    // BTools needs at least 1 input to work properly
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, 1),
    HID_INPUT(CONSTANT, VARIABLE, ABSOLUTE),
    HID_END_COLLECTION(APPLICATION),
};

bool leds_callback(uint16_t length) {
    if (length != NUMBER_OF_LIGHTS + 1) return false;
    USBDevice.recvControl(&hid_led_data, NUMBER_OF_LIGHTS);
    last_hid = millis();
    hid_dirty = true;

    return true;
}

HIDLeds_::HIDLeds_(void) {
    static HIDSubDescriptor node(_hidReportLEDs, sizeof(_hidReportLEDs));
    static HIDCallback callback = { &leds_callback, HID_REPORTID_LEDS, NULL };

    CustomHID().AppendDescriptor(&node);
    CustomHID().AppendCallback(&callback);
}
void HIDLeds_::begin(void) { return; }

HIDLeds_ HIDLeds;
