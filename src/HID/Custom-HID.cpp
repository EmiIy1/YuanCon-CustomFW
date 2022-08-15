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

#include "Lighting.h"

CustomHID_& CustomHID() {
    static CustomHID_ obj;
    return obj;
}

int CustomHID_::getInterface(uint8_t* interfaceCount) {
    *interfaceCount += 1;  // uses 1
    HIDDescriptor hidInterface = {
        D_INTERFACE(pluggedInterface, 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE,
                    HID_PROTOCOL_NONE),
        D_HIDREPORT(descriptorSize),
        D_ENDPOINT(USB_ENDPOINT_IN(pluggedEndpoint), USB_ENDPOINT_TYPE_INTERRUPT, 0x40, 0x01)
    };
    return USBDevice.sendControl(&hidInterface, sizeof(hidInterface));
}

int CustomHID_::getDescriptor(USBSetup& setup) {
    // Check if this is a HID Class Descriptor request
    if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) return 0;
    if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) return 0;

    // In a HID Class Descriptor wIndex cointains the interface number
    if (setup.wIndex != pluggedInterface) return 0;

    int total = 0;
    HIDSubDescriptor* node;
    USBDevice.packMessages(true);
    for (node = rootNode; node; node = node->next) {
        int res = USBDevice.sendControl(node->data, node->length);
        if (res == -1) return -1;
        total += res;
    }
    USBDevice.packMessages(false);
    return total;
}

void CustomHID_::AppendCallback(HIDCallback* callback) {
    if (!rootCallback) {
        rootCallback = callback;
    } else {
        HIDCallback* current = rootCallback;
        while (current->next) current = current->next;
        current->next = callback;
    }
}

void CustomHID_::AppendDescriptor(HIDSubDescriptor* node) {
    if (!rootNode) {
        rootNode = node;
    } else {
        HIDSubDescriptor* current = rootNode;
        while (current->next) current = current->next;
        current->next = node;
    }
    descriptorSize += node->length;
}

int CustomHID_::SendReport(uint8_t id, const void* data, int len) {
    uint8_t p[64];
    p[0] = id;
    memcpy(&p[1], data, len);
    return USBDevice.send(pluggedEndpoint, p, len + 1);
}

bool CustomHID_::setup(USBSetup& setup) {
    if (pluggedInterface != setup.wIndex) return false;

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
                HIDCallback* current = rootCallback;
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
    : PluggableUSBModule(1, 1, epType),
      rootCallback(NULL),
      rootNode(NULL),
      descriptorSize(0),
      protocol(1),
      idle(1) {
    epType[0] = USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_IN(0);
    ;
    PluggableUSB().plug(this);
}

int CustomHID_::begin(void) { return 0; }
