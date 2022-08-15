#include "MiniMouse.h"

#include <hid_def.h>

#include "Custom-HID.h"
#include "IDs.h"

static const uint8_t _hidMultiReportDescriptorMiniMouse[] PROGMEM = {
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(MOUSE),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_MINI_MOUSE),

    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(X),
    HID_USAGE(Y),
    HID_LOGICAL_MINIMUM(2, -0x8000),
    HID_LOGICAL_MAXIMUM(2, 0x7fff),
    HID_REPORT_SIZE(16),
    HID_REPORT_COUNT(2),
    HID_INPUT(DATA, VARIABLE, RELATIVE),

    HID_END_COLLECTION(APPLICATION),
};

MiniMouse_::MiniMouse_(void) {
    static HIDSubDescriptor node(_hidMultiReportDescriptorMiniMouse,
                                 sizeof(_hidMultiReportDescriptorMiniMouse));

    CustomHID().AppendDescriptor(&node);
    report.x_axis = report.y_axis = 0;
}
void MiniMouse_::SendReport(void* data, int length) {
    CustomHID().SendReport(HID_REPORTID_MINI_MOUSE, data, length);
}

void MiniMouse_::move(int16_t dx, int16_t dy) {
    if (!(dx || dy)) return;
    report.x_axis = dx;
    report.y_axis = dy;
    SendReport(&report, sizeof(report));
}
void MiniMouse_::begin(void) { return; }

MiniMouse_ MiniMouse;
