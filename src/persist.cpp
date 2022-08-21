#include "persist.h"

#define FLASH_DEBUG 0
#include <FlashStorage_SAMD.h>

// During development it helps to not burn through flash cycles needlessly
// #define DISABLE_PERSIST

const persistent_data_t default_con_state {
    .led_mode = {
        led_laser_mode_white,
    },
    .auto_hid = true,
    .reactive_buttons = true,
    .con_mode = con_mode_mixed,
    .keymap = { 'd', 'f', 'j', 'k', 'v', 'n', 't', 'q', 'a', 'z' },
    .button_lights = led_button_mode_live,
    .zone_colours = {
        CHSV(0, 255, 255),
        CHSV(0, 255, 255),
        CHSV(0, 255, 255),
        CHSV(0, 255, 255),
        CHSV(0, 255, 255),
        CHSV(0, 255, 255),
    },
    .zone_modes = {
        led_zone_mode_rainbow,
        led_zone_mode_rainbow,
        led_zone_mode_rainbow,
        led_zone_mode_rainbow,
        led_zone_mode_rainbow,
        led_zone_mode_rainbow,
    },

    // Up to 32 characters
    .large_macros = {
        { 0, { 0 } },
        { 0, { 0 } },
    },
    // Up to 10 characters
    .short_macros = {
        { 0, { 0 } },
        { 0, { 0 } },
        { 0, { 0 } },
        { 0, { 0 } },
    },
    // Single character (typed with a tiny_macro_speed delay), or a sequence macro from above
    .tiny_macro_speed = 50,
    .macro_layer = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    // seconds
    .led_dim = 60 * 5,  // Dim after 5 minutes
    .led_timeout = 60 * 15,  // Turn off after 15 minutes
    .led_brightness = 127,
};
persistent_data_t con_state = default_con_state;

void load_con_state() {
#ifndef DISABLE_PERSIST
    persistent_data_t loaded_data;
    uint8_t version;
    EEPROM.get(0, version);
    if (version != PERSIST_DATA_VERSION) {
        // TODO: How should we report this failure back to users?
        // TODO: Should we write data migrators in the future?
        return;
    }
    EEPROM.get(1, con_state);
#endif
}
void save_con_state() {
#ifndef DISABLE_PERSIST
    // Reads are cheap, writes are expensive. Let's check we actually have any reason to write
    // before we do!
    uint8_t version;
    EEPROM.get(0, version);
    if (version == PERSIST_DATA_VERSION) {
        persistent_data_t check_data;
        EEPROM.get(1, check_data);

        if (memcmp(&check_data, &con_state, sizeof con_state) == 0) return;
    }

    EEPROM.setCommitASAP(false);
    EEPROM.put(0, PERSIST_DATA_VERSION);
    EEPROM.put(1, con_state);
    EEPROM.commit();
#endif
}
