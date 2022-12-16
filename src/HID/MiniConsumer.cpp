#include "MiniConsumer.h"

#include <hid_def.h>

#include "Custom-HID.h"
#include "Keymap.h"

static const uint8_t _hidMultiReportDescriptorMiniConsumer[] PROGMEM = {
    HID_USAGE_PAGE(CONSUMER),
    HID_USAGE(CONSUMER_CONTROL),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_MINI_CONTROL),

    // 10 keys
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(2, 0x3ff),
    HID_USAGE_MINIMUM(1, 0),
    HID_USAGE_MAXIMUM(2, 0x3ff),
    HID_REPORT_SIZE(16),
    HID_REPORT_COUNT(1),
    HID_INPUT(DATA),

    HID_END_COLLECTION(APPLICATION),
};

MiniConsumer_::MiniConsumer_(void) {
    static HIDSubDescriptor node(_hidMultiReportDescriptorMiniConsumer,
                                 sizeof(_hidMultiReportDescriptorMiniConsumer));

    CustomHID().AppendDescriptor(&node, HID_INTERFACE_CONTROL);
    report.button = 0;
}
int MiniConsumer_::SendReport(void* data, int length) {
    return CustomHID().SendReport(HID_REPORTID_MINI_CONTROL, data, length);
}
int MiniConsumer_::write(void) {
    if (!dirty && release_next) {
        release();
        release_next = false;
    }

    if (!dirty) return 0;
    dirty = false;
    return SendReport(&report, sizeof report);
}

bool MiniConsumer_::tap(ConsumerKeycode key) {
    if (!press(key)) return false;
    release_next = true;
    return true;
};
bool MiniConsumer_::press(ConsumerKeycode key) {
    if (report.button) return false;
    report.button = key;
    return true;
};
void MiniConsumer_::release() {
    report.button = 0;
    dirty = true;
};
void MiniConsumer_::begin(void) { return; }

MiniConsumer_ MiniConsumer;
