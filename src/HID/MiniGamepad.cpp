#include "MiniGamepad.h"

#include <hid_def.h>

#include "Custom-HID.h"

static const uint8_t _hidMultiReportDescriptorMiniGamepad[] PROGMEM = {
    /* Gamepad with 10 buttons and 2 axis*/
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(JOYSTICK),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_MINI_GAMEPAD),

#ifdef STOCK_MIMICK
    HID_USAGE_PAGE(BUTTON),
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, 32),
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(1, 1),
    HID_PHYSICAL_MINIMUM(1, 0),
    HID_PHYSICAL_MAXIMUM(1, 1),
    HID_UNIT(NONE),
    HID_UNIT_EXPONENT(0),
    HID_REPORT_SIZE(1),
    HID_REPORT_COUNT(32),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(UNDEFINED),

    HID_COLLECTION(PHYSICAL),
    HID_USAGE(X),
    HID_LOGICAL_MAXIMUM(4, 0x7fff),
    HID_PHYSICAL_MAXIMUM(1, 0),
    HID_REPORT_SIZE(16),
    HID_REPORT_COUNT(1),

    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE(Y),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE(RX),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE(RY),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE(Z),

    HID_LOGICAL_MAXIMUM(1, 127),
    HID_REPORT_SIZE(8),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE(RZ),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_END_COLLECTION(PHYSICAL),
    HID_USAGE(HAT_SWITCH),
    HID_LOGICAL_MINIMUM(1, 1),
    HID_LOGICAL_MAXIMUM(1, 8),
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, 8),
    HID_REPORT_SIZE(4),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_USAGE(HAT_SWITCH),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
#else
    // X,Y axis
    HID_COLLECTION(LOGICAL),
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(4, 0xffff),
    HID_PHYSICAL_MINIMUM(1, 0),
    HID_PHYSICAL_MAXIMUM(4, 0xffff),
    HID_REPORT_SIZE(16),
    HID_REPORT_COUNT(2),
    HID_USAGE(X),
    HID_USAGE(Y),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE, BIT_FIELD),

    // Buttons
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(1, 1),
    HID_PHYSICAL_MINIMUM(1, 0),
    HID_PHYSICAL_MAXIMUM(1, 1),
    HID_REPORT_SIZE(1),
    HID_REPORT_COUNT(16),
    HID_USAGE_PAGE(BUTTON),
    HID_USAGE_MINIMUM(1, 1),
    HID_USAGE_MAXIMUM(1, 10),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
    HID_END_COLLECTION(LOGICAL),
#endif

    HID_END_COLLECTION(APPLICATION),
};

MiniGamepad_::MiniGamepad_(void) {
    static HIDSubDescriptor node(_hidMultiReportDescriptorMiniGamepad,
                                 sizeof(_hidMultiReportDescriptorMiniGamepad));

    CustomHID().AppendDescriptor(&node, HID_INTERFACE_GAMEPAD);

    report.vol_x = report.vol_y = report.buttons = 0;
    last_report.vol_x = last_report.vol_y = last_report.buttons = 0;
}
void MiniGamepad_::SendReport(void* data, int length) {
    CustomHID().SendReport(HID_REPORTID_MINI_GAMEPAD, data, length);
}
void MiniGamepad_::write(void) {
    if (last_report.vol_x != report.vol_x || last_report.vol_y != report.vol_y ||
        last_report.buttons != report.buttons) {
        SendReport(&report, sizeof(report));

        last_report.buttons = report.buttons;
        last_report.vol_x = report.vol_x;
        last_report.vol_y = report.vol_y;
    }
}
void MiniGamepad_::begin(void) { return; }

MiniGamepad_ MiniGamepad;
