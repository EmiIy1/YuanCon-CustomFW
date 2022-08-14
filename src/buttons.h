#include <Arduino.h>

extern uint16_t debounce[16];
extern uint16_t buttons;
extern uint16_t posedge_buttons;
extern uint16_t negedge_buttons;

void read_buttons();
void write_button_leds();
