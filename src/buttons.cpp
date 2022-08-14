#include "buttons.h"

#include "HID_lighting.h"

#include "pins.h"

// Debounce is performed by taking an average over this window
// Therefore, latency is ((len(debounce) / 2) * loop_time)ms -> 8ms
uint16_t debounce[16];
uint16_t buttons;
uint16_t posedge_buttons;
uint16_t negedge_buttons;

void read_buttons() {
    // Read buttons
    uint16_t new_buttons = 0;
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        if (!digitalRead(PinConf::buttons[i].btn)) new_buttons |= 1 << i;
    }

    // Propagate debouncing
    for (uint8_t i = 1; i < len(debounce); i++) {
        debounce[i] = debounce[i - 1];
    }
    debounce[0] = new_buttons;

    // Calulcate majorities
    uint16_t debounced_buttons = 0;
    for (uint8_t i = 0; i < len(PinConf::buttons); i++) {
        uint8_t sum = 0;
        for (uint8_t j = 0; j < len(debounce); j++) {
            if (debounce[j] & (1 << i)) sum++;
        }
        if (sum >= len(debounce) / 2) debounced_buttons |= (1 << i);
    }

    // Update global state
    posedge_buttons = new_buttons & ~buttons;
    negedge_buttons = buttons & ~new_buttons;
    buttons = new_buttons;
}
