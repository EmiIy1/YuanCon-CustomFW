#include "MiniKeyboard.h"

#include <hid_def.h>

#include "Custom-HID.h"
#include "IDs.h"
#include "Keymap.h"

static const uint8_t _hidMultiReportDescriptorMiniKeyboard[] PROGMEM = {
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(KEYBOARD),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_MINI_KEYBOARD),

    HID_USAGE_PAGE(KEYBOARD),

    // 10 keys
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(2, 231),
    HID_USAGE_MINIMUM(1, 0),
    HID_USAGE_MAXIMUM(1, 231),
    HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(10),
    HID_INPUT(DATA, ARRAY, ABSOLUTE),

    HID_END_COLLECTION(APPLICATION),
};

MiniKeyboard_::MiniKeyboard_(void) {
    static HIDSubDescriptor node(_hidMultiReportDescriptorMiniKeyboard,
                                 sizeof(_hidMultiReportDescriptorMiniKeyboard));

    CustomHID().AppendDescriptor(&node);
    depressed = 0;
    dirty = false;
}
void MiniKeyboard_::SendReport(void* data, int length) {
    CustomHID().SendReport(HID_REPORTID_MINI_KEYBOARD, data, length);
}
bool MiniKeyboard_::press(uint8_t key) {
    // asciimap includes modifiers. we're not going to do that.
    press((KeyboardKeycode)(_asciimap[key] & 0xff));
}
bool MiniKeyboard_::press(KeyboardKeycode key) {
    if (depressed == sizeof report.buttons) return false;
    for (uint8_t i = 0; i < depressed; i++) {
        if (report.buttons[i] == key) return true;
    }
    dirty = true;
    report.buttons[depressed++] = key;
    return true;
};
void MiniKeyboard_::release(uint8_t key) {
    release((KeyboardKeycode)(_asciimap[key] & 0xff));
}
void MiniKeyboard_::release(KeyboardKeycode key) {
    bool seen = false;
    for (uint8_t i = 0; i < depressed; i++) {
        if (seen) {
            if (i != sizeof report.buttons) report.buttons[i] = report.buttons[i + 1];
        } else if (report.buttons[i] == key) {
            if (i != sizeof report.buttons) report.buttons[i] = report.buttons[i + 1];
            seen = true;
        }
    }
    if (seen) {
        dirty = true;
        report.buttons[--depressed] = 0;
    }
};
void MiniKeyboard_::releaseAll(void) {
    if (!depressed) return;
    depressed = 0;
    dirty = true;
    memset(report.buttons, 0, sizeof report.buttons);
}

void MiniKeyboard_::write(void) {
    if (dirty) SendReport(&report, sizeof(report));
    dirty = false;
}
void MiniKeyboard_::begin(void) { return; }

MiniKeyboard_ MiniKeyboard;
