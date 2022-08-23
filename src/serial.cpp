#include "serial.h"

#include "HID/MiniKeyboard.h"
#include "persist.h"

#define SamBaReadWord(variable)                                                            \
    do {                                                                                   \
        for (uint8_t i = 8; i--;) {                                                        \
            /* we're only going to support upper case hex for now, and be very trusting */ \
            uint8_t chr = SerialUSB.read();                                                \
            if (chr >= 'A')                                                                \
                variable |= ((chr - 'A') + 10) << (i * 4);                                 \
            else                                                                           \
                variable |= (chr - '0') << (i * 4);                                        \
        }                                                                                  \
    } while (0)

void do_samba() {
    uint8_t prefix = SerialUSB.read();

    switch (prefix) {
        case 'N':
            // Switch to non-interactive mode;
            SerialUSB.write("\r\n");
            return;
        case 'V':
            // Get version string
            SerialUSB.write("YuanCon " __DATE__ " " __TIME__ "\r\n");
            return;
        case 'w': {
            // Read word
            uint32_t address = 0;
            SamBaReadWord(address);
            SerialUSB.read();  // ','
            SerialUSB.read();  // '4'
            SerialUSB.read();  // '#'

            uint32_t value = *(uint32_t *)(address);

            for (uint8_t i = 0; i < 4; i++) SerialUSB.write((value >> (i * 8)) & 0xff);

            // Would write interactively
            // for (uint8_t i = 8; i--;) {
            //     uint8_t nibble = ((value & (0b1111 << (i * 4))) >> (i * 4));
            //     if (nibble > 9)
            //         SerialUSB.write('A' + (nibble - 10));
            //     else
            //         SerialUSB.write('0' + nibble);
            // }
        }
            return;
        case 'S': {
            // Write lots of data
            uint32_t address = 0;
            SamBaReadWord(address);
            SerialUSB.read();  // ','
            uint32_t length = 0;
            SamBaReadWord(length);
            SerialUSB.read();  // '#'

            while (length) {
                *((uint8_t *)address) = SerialUSB.read();
                address++;
                length--;
            }
        }
            return;
        case 'W': {
            // Write word
            uint32_t address = 0;
            SamBaReadWord(address);
            SerialUSB.read();  // ','
            uint32_t word_ = 0;
            SamBaReadWord(word_);
            SerialUSB.read();  // '#'

            *((uint32_t *)address) = word_;
        }
            return;
    }
}

void do_serial() {
    if (!SerialUSB.available()) return;

    if (SerialUSB.baud() == 921600) return do_samba();

    uint8_t prefix = SerialUSB.read();
    switch (prefix) {
            // case 'R': {
            //     // Force a reboot into the bootloader by pretending we double tapped reset

            //     //
            //     https://github.com/sparkfun/Arduino_Boards/blob/682926ef72078d7939c12ea886f20e48cd901cd3/sparkfun/samd/bootloaders/zero/board_definitions_sparkfun_samd21dev.h#L38
            //     constexpr size_t BOOT_DOUBLE_TAP_ADDRESS = 0x20007FFCul;
            //     constexpr uint32_t DOUBLE_TAP_MAGIC = 0x07738135;
            //     *((uint32_t *)BOOT_DOUBLE_TAP_ADDRESS) = DOUBLE_TAP_MAGIC;
            // }

        case 'r':
            // General reboot
            NVIC_SystemReset();
            break;
        case 's':
            // Get board config
            SerialUSB.write(PERSIST_DATA_VERSION);
            // TODO: This is going to overflow 255 at some point!
            SerialUSB.write(sizeof con_state);
            SerialUSB.write((uint8_t *)&con_state, sizeof con_state);
            break;
        case 'c':
            // Set board config
            SerialUSB.readBytes((uint8_t *)&con_state, sizeof con_state);
            SerialUSB.write('c');
            break;
        case 'l':
            // Clear board config
            memcpy(&con_state, &default_con_state, sizeof con_state);
            save_con_state();
            // Keymap possibly changed, so unpress everything
            MiniKeyboard.releaseAll();
            MiniKeyboard.write();
            // TODO: Unpress gamepad buttons, once those are configurable
            SerialUSB.write('l');
            break;
        case 'C':
            // Commit board config
            save_con_state();
            load_con_state();
            SerialUSB.write('C');
            break;
        default:
            break;
    }
}