#include <Arduino.h>

extern volatile uint16_t vol_x, vol_y;
extern volatile int8_t vol_delta_x, vol_delta_y;

extern volatile int8_t vol_x_dir, vol_y_dir;
// LEDs update less frequently so need their own vars to reset
extern volatile int8_t vol_x_dir_led, vol_y_dir_led;

void setup_vol();
