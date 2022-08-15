#include "buttons.h"

#include "HID/Lighting.h"
#include "pins.h"

unsigned long debounce[len(PinConf::buttons)];
constexpr unsigned long debounce_time = 30000;  // us = 3ms
uint16_t buttons;
uint16_t posedge_buttons;
uint16_t negedge_buttons;

void read_buttons() {
    auto now = micros();
    uint16_t new_buttons = 0;
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (digitalRead(PinConf::buttons[i].btn)) {
            if (now - debounce[i] > debounce_time) debounce[i] = 0;
        } else {
            debounce[i] = micros();
        }

        if (debounce[i]) new_buttons |= (1 << i);
    }

    // Update global state
    posedge_buttons = new_buttons & ~buttons;
    negedge_buttons = buttons & ~new_buttons;
    buttons = new_buttons;
}
