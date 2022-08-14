#include "vol.h"

#include "pins.h"

volatile int8_t vol_x_dir = 0, vol_y_dir = 0;
volatile int8_t vol_x_dir_led = 0, vol_y_dir_led = 0;

void ai0() {
    if (digitalRead(PinConf::vol_x.g2) == LOW)
        vol_x_dir = -1;
    else
        vol_x_dir = 1;
}
void ai1() {
    if (digitalRead(PinConf::vol_x.g1) == LOW)
        vol_x_dir = 1;
    else
        vol_x_dir = -1;
}
void ai3() {
    if (digitalRead(PinConf::vol_y.g2) == LOW)
        vol_y_dir = -1;
    else
        vol_y_dir = 1;
}
void ai4() {
    if (digitalRead(PinConf::vol_y.g1) == LOW)
        vol_y_dir = 1;
    else
        vol_y_dir = -1;
}

void setup_vol() {
    pinMode(PinConf::vol_x.g1, INPUT_PULLUP);
    pinMode(PinConf::vol_x.g2, INPUT_PULLUP);
    pinMode(PinConf::vol_y.g1, INPUT_PULLUP);
    pinMode(PinConf::vol_y.g2, INPUT_PULLUP);
    attachInterrupt(PinConf::vol_x.g1, ai0, RISING);
    attachInterrupt(PinConf::vol_x.g2, ai1, RISING);
    attachInterrupt(PinConf::vol_y.g1, ai3, RISING);
    attachInterrupt(PinConf::vol_y.g2, ai4, RISING);
}