#pragma once

#include <HID.h>
#include <vendor.h>

typedef struct ATTRIBUTE_PACKED {
    uint8_t buttons[10];
} HID_MiniKeyboardReport_Data_t;

class MiniKeyboard_ {
    uint8_t depressed;
    bool dirty;

   public:
    HID_MiniKeyboardReport_Data_t report;

    MiniKeyboard_(void);
    void SendReport(void* data, int length);
    bool press(uint8_t key);
    void release(uint8_t key);
    void releaseAll(void);
    void write(void);
    void begin(void);
};

extern MiniKeyboard_ MiniKeyboard;
