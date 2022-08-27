#pragma once

#include <HID.h>
#include <vendor.h>

// #define STOCK_MIMICK

typedef struct ATTRIBUTE_PACKED {
#ifdef STOCK_MIMICK
    uint16_t buttons;
    uint16_t buttons_high;

    int16_t vol_x;
    int16_t vol_y;
    uint16_t rx;
    uint16_t ry;
    uint16_t z;
    uint8_t rz;
    uint8_t hat_switch;
#else
    int16_t vol_x;
    int16_t vol_y;
    uint16_t buttons;
#endif
} HID_MiniGamepadReport_Data_t;

class MiniGamepad_ {
   public:
    HID_MiniGamepadReport_Data_t report;
    HID_MiniGamepadReport_Data_t last_report;

    MiniGamepad_(void);
    void SendReport(void* data, int length);
    void write(void);
    void begin(void);
};

extern MiniGamepad_ MiniGamepad;
