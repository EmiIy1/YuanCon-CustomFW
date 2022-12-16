#pragma once

#include <HID.h>
#include <vendor.h>

typedef struct ATTRIBUTE_PACKED {
    int16_t x_axis;
    int16_t y_axis;
} HID_MiniMouseReport_Data_t;

class MiniMouse_ {
   public:
    bool moved = false;
    HID_MiniMouseReport_Data_t report;

    MiniMouse_(void);
    void begin(void);
    int SendReport(void* data, int length);
    int write(void);

    void move(int16_t dx, int16_t dy);
};

extern MiniMouse_ MiniMouse;
