#pragma once

#ifdef USB_MANUFACTURER
#undef USB_MANUFACTURER
#endif
#define USB_MANUFACTURER "Hello world"
#ifdef USB_PRODUCT
#undef USB_PRODUCT
#endif
#define USB_PRODUCT "the quick brown fox"
#ifdef USB_PRODUCT_NAME
#undef USB_PRODUCT_NAME
#endif
#define USB_PRODUCT_NAME "the brown fox"
#ifdef USB_VENDOR_DESCRIPTOR
#undef USB_VENDOR_DESCRIPTOR
#endif
#define USB_VENDOR_DESCRIPTOR "the fox"

#include <Arduino.h>
#include <HID-Project.h>

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
