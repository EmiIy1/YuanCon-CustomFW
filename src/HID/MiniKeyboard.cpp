#include "MiniKeyboard.h"

#include <hid_def.h>

#include "Custom-HID.h"
#include "Keymap.h"

static const uint8_t _hidMultiReportDescriptorMiniKeyboard[] PROGMEM = {
    HID_USAGE_PAGE(GENERIC_DESKTOP),
    HID_USAGE(KEYBOARD),
    HID_COLLECTION(APPLICATION),
    HID_REPORT_ID(HID_REPORTID_MINI_KEYBOARD),

    HID_USAGE_PAGE(KEYBOARD),

    // 10+1 keys (10 used for buttons, 1 in case we have all buttons somehow pressed while macroing)
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(2, 231),
    HID_USAGE_MINIMUM(1, 0),
    HID_USAGE_MAXIMUM(1, 231),
    HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(MiniKeyboard_Keys),
    HID_INPUT(DATA, ARRAY, ABSOLUTE),

    // Keyboard modifiers
    HID_USAGE_MINIMUM(1, 0xe0),
    HID_USAGE_MAXIMUM(1, 0xe7),
    HID_LOGICAL_MINIMUM(1, 0),
    HID_LOGICAL_MAXIMUM(1, 1),
    HID_REPORT_SIZE(1),
    HID_REPORT_COUNT(8),
    HID_INPUT(DATA, VARIABLE, ABSOLUTE),

    HID_END_COLLECTION(APPLICATION),
};

MiniKeyboard_::MiniKeyboard_(void) {
    static HIDSubDescriptor node(_hidMultiReportDescriptorMiniKeyboard,
                                 sizeof(_hidMultiReportDescriptorMiniKeyboard));

    CustomHID().AppendDescriptor(&node, HID_INTERFACE_KEYBOARD);
    depressed = 0;
    dirty = false;
}
int MiniKeyboard_::SendReport(void* data, int length) {
    return CustomHID().SendReport(HID_REPORTID_MINI_KEYBOARD, data, length);
}
int MiniKeyboard_::write(void) {
    if (dirty) {
        dirty = false;
        return SendReport(&report, sizeof(report));
    }
    dirty = false;
    return 0;
}

void MiniKeyboard_::add_modifiers(uint8_t modifiers) {
    for (uint8_t i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) modifier_count[i]++;
    }
    if (modifiers & (report.modifiers ^ modifiers)) {
        report.modifiers |= modifiers;
        dirty = true;
    }
}
void MiniKeyboard_::remove_modifiers(uint8_t modifiers) {
    for (uint8_t i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) {
            if ((--modifier_count[i]) == 0) {
                report.modifiers &= ~(1 << i);
                dirty = true;
            }
        }
    }
}

bool MiniKeyboard_::press(uint8_t key) {
    uint16_t mapped = _asciimap[key];
    add_modifiers(mapped >> 8);
    return press((KeyboardKeycode)(mapped & 0xff));
}
bool MiniKeyboard_::press(KeyboardKeycode key) {
    if (depressed == MiniKeyboard_Keys) return false;
    for (uint8_t i = 0; i < depressed; i++) {
        if (report.buttons[i] == key) return true;
    }
    dirty = true;
    report.buttons[depressed++] = key;
    return true;
};
void MiniKeyboard_::release(uint8_t key) {
    uint16_t mapped = _asciimap[key];
    remove_modifiers(mapped >> 8);
    release((KeyboardKeycode)(mapped & 0xff));
}
void MiniKeyboard_::release(KeyboardKeycode key) {
    bool seen = false;
    for (uint8_t i = 0; i < depressed; i++) {
        if (seen) {
            if (i != MiniKeyboard_Keys) report.buttons[i] = report.buttons[i + 1];
        } else if (report.buttons[i] == key) {
            if (i != MiniKeyboard_Keys) report.buttons[i] = report.buttons[i + 1];
            seen = true;
        }
    }
    if (seen) {
        dirty = true;
        report.buttons[--depressed] = 0;
    }
};
void MiniKeyboard_::releaseAll(void) {
    if (!depressed && !report.modifiers) return;

    depressed = 0;
    report.modifiers = 0;
    memset(modifier_count, 0, sizeof modifier_count);
    memset(report.buttons, 0, sizeof report.buttons);
    dirty = true;
}

void MiniKeyboard_::begin(void) { return; }

MiniKeyboard_ MiniKeyboard;
