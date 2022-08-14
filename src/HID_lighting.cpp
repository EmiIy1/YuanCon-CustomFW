#include "HID_lighting.h"

led_data_t hid_led_data;
bool hid_dirty = false;
unsigned long last_hid = 0;

static const uint8_t PROGMEM _hidReportLEDs[] = {
    0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
    0x09, 0x00,  // USAGE (Undefined)
    0xa1, 0x01,  // COLLECTION (Application)
    // Globals
    0x95, NUMBER_OF_LIGHTS,  //   REPORT_COUNT
    0x75, 0x08,              //   REPORT_SIZE (8)
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,        //   LOGICAL_MAXIMUM (255)
    0x05, 0x0a,              //   USAGE_PAGE (Ordinals)
    // Locals
    0x19, 0x01,              //   USAGE_MINIMUM (Instance 1)
    0x29, NUMBER_OF_LIGHTS,  //   USAGE_MAXIMUM (Instance n)
    0x91, 0x02,              //   OUTPUT (Data,Var,Abs)
    // BTools needs at least 1 input to work properly
    0x19, 0x01,  //   USAGE_MINIMUM (Instance 1)
    0x29, 0x01,  //   USAGE_MAXIMUM (Instance 1)
    0x81, 0x03,  //   INPUT (Cnst,Var,Abs)
    0xc0         // END_COLLECTION
};

HIDLED_::HIDLED_(void) : PluggableUSBModule(1, 1, epType) {
    epType[0] = EP_TYPE_INTERRUPT_IN;
    PluggableUSB().plug(this);
}
int HIDLED_::getInterface(uint8_t* interfaceCount) {
    *interfaceCount += 1;  // uses 1
    HIDDescriptor hidInterface = {
        D_INTERFACE(pluggedInterface, 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE,
                    HID_PROTOCOL_NONE),
        D_HIDREPORT(sizeof(_hidReportLEDs)),
        D_ENDPOINT(USB_ENDPOINT_IN(pluggedEndpoint), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE,
                   0x10),
    };
    return USBDevice.sendControl(&hidInterface, sizeof(hidInterface));
}

int HIDLED_::getDescriptor(USBSetup& setup) {
    if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) return 0;
    if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) return 0;
    if (setup.wIndex != pluggedInterface) return 0;
    return USBDevice.sendControl(_hidReportLEDs, sizeof(_hidReportLEDs));
}

bool HIDLED_::setup(USBSetup& setup) {
    if (pluggedInterface != setup.wIndex) return false;

    uint8_t request = setup.bRequest;
    uint8_t requestType = setup.bmRequestType;

    if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
        return true;
    }

    if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
        if (request == HID_SET_REPORT) {
            if (setup.wValueH == HID_REPORT_TYPE_OUTPUT && setup.wLength == NUMBER_OF_LIGHTS) {
                USBDevice.recvControl(hid_led_data.raw, NUMBER_OF_LIGHTS);
                last_hid = millis();
                hid_dirty = true;
                return true;
            }
        }
    }

    return false;
}

void HIDLED_::begin(void) { return; }

HIDLED_ HIDLeds;
