#include <vendor.h>

#include "pins.h"

// extern volatile uint16_t vol_x, vol_y;
// extern volatile int8_t vol_delta_x, vol_delta_y;

// extern volatile int8_t vol_x_dir, vol_y_dir;

// LEDs update less frequently so need their own vars to reset
extern volatile int8_t vol_x_dir_led, vol_y_dir_led;

void setup_vol();

template <pin_size_t g1, pin_size_t g2, bool double_interrupt>
class Vol_ {
   private:
    // By tracking direction here we avoid needing to handle over and underflow when checking the
    // direction. The handful of extra cycles in these ISRs are fine.

    static void isr1(void) {
        __disable_irq();
        // if (readVolX1 == readVolX2) {
        if (digitalRead(g1) == digitalRead(g2)) {
            val++;
            dir = 1;
            if (delta < 128) delta++;
        } else {
            val--;
            dir = -1;
            if (delta > -127) delta--;
        }
        __enable_irq();
    };
    static void isr2(void) {
        __disable_irq();
        // if (readVolX1 != readVolX2) {
        if (digitalRead(g1) != digitalRead(g2)) {
            val++;
            dir = 1;
            if (delta < 128) delta++;
        } else {
            val--;
            dir = -1;
            if (delta > -127) delta--;
        }
        __enable_irq();
    };

   public:
    static uint16_t val;
    static int8_t dir;
    static int8_t delta;

    void ticked() { dir = delta = 0; }

    Vol_() {
        val = dir = delta = 0;

        pinMode(g1, INPUT_PULLUP);
        attachInterrupt(g1, isr1, CHANGE);
        if (double_interrupt) {
            pinMode(g2, INPUT_PULLUP);
            attachInterrupt(g2, isr2, CHANGE);
        }
    }
};

extern Vol_<PinConf::vol_x.g1, PinConf::vol_x.g2, PinConf::kobs_two_interrupts> VolX;
extern Vol_<PinConf::vol_y.g1, PinConf::vol_y.g2, PinConf::kobs_two_interrupts> VolY;
