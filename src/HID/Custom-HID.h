#include <HID.h>
#include "IDs.h"

typedef struct HIDCallback_ {
    bool (*callback)(uint16_t length);
    uint8_t report_id;
    struct HIDCallback_* next;
} HIDCallback;

class CustomHID_ : public PluggableUSBModule {
   public:
    CustomHID_(void);
    int begin(void);
    int SendReport(uint8_t id, const void* data, int len);
    void AppendDescriptor(HIDSubDescriptor* node, uint8_t interface);
    void AppendCallback(HIDCallback* callback, uint8_t interface);
    uint8_t getShortName(char* name);

   protected:
    int getInterface(uint8_t* interfaceCount);
    int getDescriptor(USBSetup& setup);
    bool setup(USBSetup& setup);

   private:
    unsigned int epType[1];

    uint8_t interfaces[HID_INTERFACES];
    HIDCallback_* rootCallback[HID_INTERFACES];
    HIDSubDescriptor* rootNode[HID_INTERFACES];
    uint16_t descriptorSize[HID_INTERFACES];

    uint8_t protocol;
    uint8_t idle;
};
CustomHID_& CustomHID();
CustomHID_& CustomHID_Lighting();
