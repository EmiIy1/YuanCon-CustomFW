#pragma once

#include <HID.h>
#include <vendor.h>

#include "HID/ConsumerMap.h"

typedef struct ATTRIBUTE_PACKED {
    uint16_t button;
} HID_MiniConsumerReport_Data_t;

class MiniConsumer_ {
   public:
    HID_MiniConsumerReport_Data_t report;

    MiniConsumer_(void);
    void SendReport(void* data, int length);
    bool press(ConsumerKeycode key);
    bool write(ConsumerKeycode key);
    void release();
    void begin(void);
};

extern MiniConsumer_ MiniConsumer;
