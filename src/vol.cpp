#include "vol.h"

#include "pins.h"

volatile int8_t vol_x_dir_led = 0, vol_y_dir_led = 0;

template <pin_size_t g1, pin_size_t g2, bool double_interrupt>
uint16_t Vol_<g1, g2, double_interrupt>::val;
template <pin_size_t g1, pin_size_t g2, bool double_interrupt>
int8_t Vol_<g1, g2, double_interrupt>::delta;
template <pin_size_t g1, pin_size_t g2, bool double_interrupt>
int8_t Vol_<g1, g2, double_interrupt>::dir;

Vol_<PinConf::vol_x.g1, PinConf::vol_x.g2, PinConf::kobs_two_interrupts> VolX;
Vol_<PinConf::vol_y.g1, PinConf::vol_y.g2, PinConf::kobs_two_interrupts> VolY;
