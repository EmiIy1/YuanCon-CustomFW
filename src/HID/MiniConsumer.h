#pragma once

#include <HID.h>
#include <vendor.h>

#include "HID/ConsumerMap.h"

typedef struct ATTRIBUTE_PACKED {
    uint16_t button;
} HID_MiniConsumerReport_Data_t;

class MiniConsumer_ {
   public:
    MiniConsumer_(void);
    void begin(void);
    int SendReport(void* data, int length);
    int write(void);

    bool tap(ConsumerKeycode key);
    bool press(ConsumerKeycode key);
    void release(void);

   private:
    HID_MiniConsumerReport_Data_t report;
    bool release_next = true;
    bool dirty = false;
};

extern MiniConsumer_ MiniConsumer;
