/* Copyright (c) 2015, Arduino LLC
**
** Original code (pre-library): Copyright (c) 2011, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*
* Copied from HID.cpp in the arduino core libraries.
* Modified as part of this custom firmware to provide HID_SET_REPORT.
*/

#include "Custom-HID.h"

#include "IDs.h"
#include "Lighting.h"

CustomHID_& CustomHID() {
    static CustomHID_ obj;
    return obj;
}

int CustomHID_::getInterface(uint8_t* interfaceCount) {
    *interfaceCount += HID_INTERFACES;

    int sent = 0;
    for (uint8_t interface = 0; interface < HID_INTERFACES; interface++) {
        interfaces[interface] = pluggedInterface + interface;

        HIDDescriptor hidInterface = {
            D_INTERFACE(interfaces[interface], 1, USB_DEVICE_CLASS_HUMAN_INTERFACE,
                        HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
            D_HIDREPORT(descriptorSize[interface]),
            D_ENDPOINT(USB_ENDPOINT_IN(pluggedEndpoint + interface), USB_ENDPOINT_TYPE_INTERRUPT,
                       0x40, 0),
        };
        sent += USBDevice.sendControl(&hidInterface, sizeof(hidInterface));
    }

    return sent;
}

static void utox8(uint32_t val, char* s) {
    for (int i = 0; i < 8; i++) {
        int d = val & 0XF;
        val = (val >> 4);

        s[7 - i] = d > 9 ? 'A' + d - 10 : '0' + d;
    }
}
uint8_t CustomHID_::getShortName(char* name) {
    name[0] = 'Y';
    name[1] = 'C';
    name[2] = 'F';
    name[3] = 'W';
    name[4] = '-';

#define SERIAL_NUMBER_WORD_0 *(volatile uint32_t*)(0x0080A00C)
#define SERIAL_NUMBER_WORD_1 *(volatile uint32_t*)(0x0080A040)
#define SERIAL_NUMBER_WORD_2 *(volatile uint32_t*)(0x0080A044)
#define SERIAL_NUMBER_WORD_3 *(volatile uint32_t*)(0x0080A048)

    utox8(SERIAL_NUMBER_WORD_0, &name[5]);
    utox8(SERIAL_NUMBER_WORD_1, &name[13]);
    utox8(SERIAL_NUMBER_WORD_2, &name[21]);
    utox8(SERIAL_NUMBER_WORD_3, &name[29]);

    name[36] = '\0';
    return 36;
}

int CustomHID_::getDescriptor(USBSetup& setup) {
    // TODO: Make this configurable
    static const uint8_t STRING_PRODUCT[] = "YuanCon";

    if (setup.wValueH == USB_STRING_DESCRIPTOR_TYPE) {
        switch (setup.wValueL) {
            case IMANUFACTURER:
                return USBDevice.sendStringDescriptor(STRING_PRODUCT, setup.wLength);
            case IPRODUCT:
                return USBDevice.sendStringDescriptor(STRING_PRODUCT, setup.wLength);
            case ISERIAL:
                return 0;
        }
        if (setup.wValueL >= HID_Strings_Base &&
            setup.wValueL < HID_Strings_Base + (sizeof HID_strings / sizeof HID_strings[0])) {
            if (setup.wValueL >= HID_strings_rgb && setup.wValueL < HID_strings_rgb_end) {
                uint8_t string_idx = setup.wValueL - HID_Strings_Base;
                string_idx -= (setup.wValueL - HID_strings_rgb) % 3;

                const char* string = HID_strings[string_idx];
                // Append R, G, or B to the string
                char* modified = strcpy((char*)malloc(strlen(string) + 3), string);
                char* end = modified + strlen(string);
                end[0] = ' ';
                switch ((setup.wValueL - HID_strings_rgb) % 3) {
                    case 0:
                        end[1] = 'R';
                        break;
                    case 1:
                        end[1] = 'G';
                        break;
                    case 2:
                    default:
                        end[1] = 'B';
                        break;
                }
                end[2] = '\0';

                bool ret = USBDevice.sendStringDescriptor((uint8_t*)modified, setup.wLength);
                free(modified);
                return ret;
            } else {
                return USBDevice.sendStringDescriptor(
                    (uint8_t*)HID_strings[setup.wValueL - HID_Strings_Base], setup.wLength);
            }
        }
        return 0;
    }

    // Check if this is a HID Class Descriptor request
    if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) return 0;
    if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) return 0;

    // In a HID Class Descriptor wIndex cointains the interface number
    uint8_t interface = 0;
    for (; interface < sizeof interfaces; interface++)
        if (interfaces[interface] == setup.wIndex) goto setup_noreturn;
    return 0;
setup_noreturn:

    // USBDevice.packMessages stores data into a 256-byte buffer. This is... too small. Far too
    // small :).
    uint8_t* buffer = (uint8_t*)malloc(descriptorSize[interface]);
    uint8_t* workBuffer = buffer;

    HIDSubDescriptor* node;
    for (node = rootNode[interface]; node; node = node->next) {
        memcpy(workBuffer, node->data, node->length);
        workBuffer += node->length;
    }
    int res = USBDevice.sendControl(buffer, descriptorSize[interface]);
    free(buffer);
    if (res == -1) return -1;
    return descriptorSize[interface];
}

void CustomHID_::AppendCallback(HIDCallback* callback, uint8_t interface) {
    if (!rootCallback[interface]) {
        rootCallback[interface] = callback;
    } else {
        HIDCallback* current = rootCallback[interface];
        while (current->next) current = current->next;
        current->next = callback;
    }
}

void CustomHID_::AppendDescriptor(HIDSubDescriptor* node, uint8_t interface) {
    if (!rootNode[interface]) {
        rootNode[interface] = node;
    } else {
        HIDSubDescriptor* current = rootNode[interface];
        while (current->next) current = current->next;
        current->next = node;
    }
    descriptorSize[interface] += node->length;
}

int CustomHID_::SendReport(uint8_t id, const void* data, int len) {
    uint8_t p[64];
    p[0] = id;
    memcpy(&p[1], data, len);
    return USBDevice.send(pluggedEndpoint, p, len + 1);
}

bool CustomHID_::setup(USBSetup& setup) {
    uint8_t interface = 0;
    for (; interface < sizeof interfaces; interface++)
        if (interfaces[interface] == setup.wIndex) goto setup_noreturn;
    return false;
setup_noreturn:

    uint8_t request = setup.bRequest;
    uint8_t requestType = setup.bmRequestType;

    if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
        if (request == HID_GET_REPORT) {
            // TODO: HID_GetReport();
            return true;
        }
        if (request == HID_GET_PROTOCOL) {
            // TODO: Send8(protocol);
            return true;
        }
        if (request == HID_GET_IDLE) {
            USBDevice.armSend(0, &idle, 1);
            return true;
        }
    }

    if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
        if (request == HID_SET_PROTOCOL) {
            // The USB Host tells us if we are in boot or report mode.
            // This only works with a real boot compatible device.
            protocol = setup.wValueL;
            return true;
        }
        if (request == HID_SET_IDLE) {
            idle = setup.wValueL;
            return true;
        }
        if (request == HID_SET_REPORT) {
            if (setup.wValueH == HID_REPORT_TYPE_OUTPUT) {
                HIDCallback* current = rootCallback[interface];
                while (current) {
                    if (current->report_id == setup.wValueL) {
                        return current->callback(setup.wLength);
                    }

                    current = current->next;
                }
            }
        }
    }

    return false;
}

CustomHID_::CustomHID_(void)
    : PluggableUSBModule(HID_INTERFACES, HID_INTERFACES, epType),
      rootCallback{ NULL },
      rootNode{ NULL },
      descriptorSize{ 0 },
      protocol(1),
      idle(1) {
    epType[0] = USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_IN(0);
    PluggableUSB().plug(this);
}

int CustomHID_::begin(void) { return 0; }
