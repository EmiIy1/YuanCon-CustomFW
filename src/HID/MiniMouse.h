#pragma once

#include <HID.h>
#include <vendor.h>

typedef struct ATTRIBUTE_PACKED {
    int16_t x_axis;
    int16_t y_axis;
} HID_MiniMouseReport_Data_t;

class MiniMouse_ {
   public:
    HID_MiniMouseReport_Data_t report;

    MiniMouse_(void);
    void SendReport(void* data, int length);
    void move(int16_t dx, int16_t dy);
    void begin(void);
};

extern MiniMouse_ MiniMouse;
