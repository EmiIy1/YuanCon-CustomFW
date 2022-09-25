#include "analog.h"

#include "persist.h"
#include "pins.h"

volatile int8_t vol_x_dir_led = 0, vol_y_dir_led = 0;
volatile int8_t vol_x_dir = 0, vol_y_dir = 0;

// Create real memory locations for static members
template <uint8_t num, pin_size_t g1, pin_size_t g2, bool double_interrupt>
uint16_t OpticalEncoder<num, g1, g2, double_interrupt>::phys_val;
template <uint8_t num, pin_size_t g1, pin_size_t g2, bool double_interrupt>
int8_t OpticalEncoder<num, g1, g2, double_interrupt>::phys_delta;
template <uint8_t num, pin_size_t g1, pin_size_t g2, bool double_interrupt>
int8_t OpticalEncoder<num, g1, g2, double_interrupt>::phys_dir;

// Create encoder instances
OpticalEncoder<0, PinConf::vol_x.g1, PinConf::vol_x.g2, PinConf::kobs_two_interrupts> VolX;
OpticalEncoder<1, PinConf::vol_y.g1, PinConf::vol_y.g2, PinConf::kobs_two_interrupts> VolY;
GenericAnalog analog_inputs[NUM_ANALOGS] = {
    VolX,
    VolY,
};

GenericAnalog::GenericAnalog(uint8_t num, uint16_t* ptr_phys_val, int8_t* ptr_phys_dir,
                             int8_t* ptr_phys_delta)
    : deadzone_delta(0),
      ptr_phys_val(ptr_phys_val),
      ptr_phys_dir(ptr_phys_dir),
      ptr_phys_delta(ptr_phys_delta),
      val(0),
      dir(0),
      delta(0) {
    // Dummy, for now!
    deadzone = con_state.analogs[num].deadzone;
    deadzone_bounceback = con_state.analogs[num].deadzone_bounceback;
    bounceback_timer = con_state.analogs[num].bounceback_timer;
    bounceback_timer_val = bounceback_timer;
}
