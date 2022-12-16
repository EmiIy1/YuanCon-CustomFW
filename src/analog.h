#pragma once
#include <vendor.h>

#include <vector>

#include "pins.h"

// LEDs update less frequently so need their own vars to reset
extern volatile int8_t vol_x_dir_led, vol_y_dir_led;
extern volatile int8_t vol_x_dir, vol_y_dir;

class GenericAnalog {
   private:
    int16_t deadzone_delta;
    uint16_t bounceback_timer_val;

    // Deadzone config
    uint16_t deadzone;
    uint8_t deadzone_bounceback;
    uint16_t bounceback_timer;  // ticks

   protected:
    uint16_t* ptr_phys_val;
    int8_t* ptr_phys_dir;
    int8_t* ptr_phys_delta;

   public:
    uint16_t val;
    int8_t dir;
    int8_t delta;

    GenericAnalog(uint8_t num, uint16_t* ptr_phys_val, int8_t* ptr_phys_dir,
                  int8_t* ptr_phys_delta);

    void tick() {
        if (deadzone == 0) {
            // Easy case; just mirror physical values into the public members
            dir = *ptr_phys_dir;
            delta = *ptr_phys_delta;
            // if (*ptr_phys_val) {
            //     (*ptr_phys_val) = 0;
            //     delta = -1;
            // } else {
            //     (*ptr_phys_val) = 1;
            //     delta = 1;
            // }
            val = *ptr_phys_val;
        } else {
            deadzone_delta += *ptr_phys_delta;

            if (deadzone_delta > deadzone) {
                delta = deadzone_delta - deadzone;
                deadzone_delta = deadzone;
                dir = 1;
                val += delta;
            } else if (deadzone_delta < -deadzone) {
                delta = deadzone_delta + deadzone;
                deadzone_delta = -deadzone;
                dir = -1;
                val += delta;
            } else {
                dir = 0;
                delta = 0;
            }

            // If we're not moving the encoder, perform bounceback
            if (*ptr_phys_delta == 0 && deadzone_bounceback != 0 &&
                (deadzone_delta == deadzone || deadzone_delta == -deadzone)) {
                if (bounceback_timer_val == 0) {
                    if (deadzone_delta < 0)
                        deadzone_delta += deadzone_bounceback;
                    else
                        deadzone_delta -= deadzone_bounceback;
                } else {
                    bounceback_timer_val--;
                }
            } else {
                bounceback_timer_val = bounceback_timer;
            }
        }

        // Reset internal state
        *ptr_phys_dir = *ptr_phys_delta = 0;
    }
};

template <uint8_t num, pin_size_t g1, pin_size_t g2, bool double_interrupt>
class OpticalEncoder : public GenericAnalog {
   private:
    // By tracking direction here we avoid needing to handle over and underflow when checking the
    // direction. The handful of extra cycles in these ISRs are fine.

    static void isr1(void) {
        __disable_irq();
        if (digitalRead(g1) == digitalRead(g2)) {
            phys_val++;
            phys_dir = 1;
            if (phys_delta < 128) phys_delta++;
        } else {
            phys_val--;
            phys_dir = -1;
            if (phys_delta > -127) phys_delta--;
        }
        __enable_irq();
    };
    static void isr2(void) {
        __disable_irq();
        if (digitalRead(g1) != digitalRead(g2)) {
            phys_val++;
            phys_dir = 1;
            if (phys_delta < 128) phys_delta++;
        } else {
            phys_val--;
            phys_dir = -1;
            if (phys_delta > -127) phys_delta--;
        }
        __enable_irq();
    };

    static uint16_t phys_val;
    static int8_t phys_dir;
    static int8_t phys_delta;

   public:
    OpticalEncoder() : GenericAnalog(num, &phys_val, &phys_dir, &phys_delta) {
        phys_val = phys_dir = phys_delta = 0;

        pinMode(g1, INPUT_PULLUP);
        attachInterrupt(g1, isr1, CHANGE);
        if (double_interrupt) {
            pinMode(g2, INPUT_PULLUP);
            attachInterrupt(g2, isr2, CHANGE);
        }
    }
};

extern GenericAnalog analog_inputs[NUM_ANALOGS];
