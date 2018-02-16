#include "VirtualHIDKeyboard.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(VIRTUAL_HID_KEYBOARD_CLASS, super);

namespace {
uint8_t reportDescriptor_[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report Id (1)
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0xe0,       //   Usage Minimum........... (224)
    0x29, 0xe7,       //   Usage Maximum........... (231)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x25, 0x01,       //   Logical Maximum......... (1)
    0x75, 0x01,       //   Report Size............. (1)
    0x95, 0x08,       //   Report Count............ (8)
    0x81, 0x02,       //   Input...................(Data, Variable, Absolute)
                      //
    0x95, 0x01,       //   Report Count............ (1)
    0x75, 0x08,       //   Report Size............. (8)
    0x81, 0x01,       //   Input...................(Constant)
                      //
    0x95, 0x06,       //   Report Count............ (6)
    0x75, 0x08,       //   Report Size............. (8)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,       //   Usage Minimum........... (0)
    0x29, 0xff,       //   Usage Maximum........... (255)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
                      //
    0x05, 0xff,       //   Usage Page (kHIDPage_AppleVendorTopCase)
    0x19, 0x03,       //   Usage Minimum........... (kHIDUsage_AV_TopCase_KeyboardFn)
    0x29, 0x03,       //   Usage Maximum........... (kHIDUsage_AV_TopCase_KeyboardFn)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x95, 0x01,       //   Report Count............ (1)
    0x75, 0x08,       //   Report Size............. (8)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
    0xc0,             // End Collection
                      //
    0x05, 0x0c,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage 1 (kHIDUsage_Csmr_ConsumerControl)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report Id (2)
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0xe0,       //   Usage Minimum........... (224)
    0x29, 0xe7,       //   Usage Maximum........... (231)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x25, 0x01,       //   Logical Maximum......... (1)
    0x75, 0x01,       //   Report Size............. (1)
    0x95, 0x08,       //   Report Count............ (8)
    0x81, 0x02,       //   Input...................(Data, Variable, Absolute)
                      //
    0x95, 0x01,       //   Report Count............ (1)
    0x75, 0x08,       //   Report Size............. (8)
    0x81, 0x01,       //   Input...................(Constant)
                      //
    0x95, 0x06,       //   Report Count............ (6)
    0x75, 0x08,       //   Report Size............. (8)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x05, 0x0c,       //   Usage Page (Consumer)
    0x19, 0x00,       //   Usage Minimum........... (0)
    0x29, 0xff,       //   Usage Maximum........... (255)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
                      //
    0x05, 0xff,       //   Usage Page (kHIDPage_AppleVendorTopCase)
    0x19, 0x03,       //   Usage Minimum........... (kHIDUsage_AV_TopCase_KeyboardFn)
    0x29, 0x03,       //   Usage Maximum........... (kHIDUsage_AV_TopCase_KeyboardFn)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x95, 0x01,       //   Report Count............ (1)
    0x75, 0x08,       //   Report Size............. (8)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
    0xc0,             // End Collection
};
}

bool VIRTUAL_HID_KEYBOARD_CLASS::handleStart(IOService* provider) {
  if (!super::handleStart(provider)) {
    return false;
  }

  // We have to set kIOHIDVirtualHIDevice to suppress Keyboard Setup Assistant.
  setProperty(kIOHIDVirtualHIDevice, kOSBooleanTrue);
  setProperty("HIDDefaultBehavior", kOSBooleanTrue);
  setProperty("AppleVendorSupported", kOSBooleanTrue);

  return true;
}

OSString* VIRTUAL_HID_KEYBOARD_CLASS::newManufacturerString() const {
  return OSString::withCString("pqrs.org");
}

OSString* VIRTUAL_HID_KEYBOARD_CLASS::newProductString() const {
  return OSString::withCString("Karabiner VirtualHIDKeyboard");
}

OSNumber* VIRTUAL_HID_KEYBOARD_CLASS::newVendorIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0x16c0), 32);
}

OSNumber* VIRTUAL_HID_KEYBOARD_CLASS::newProductIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0x27db), 32);
}

OSNumber* VIRTUAL_HID_KEYBOARD_CLASS::newPrimaryUsageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDUsage_GD_Keyboard), 32);
}

OSNumber* VIRTUAL_HID_KEYBOARD_CLASS::newPrimaryUsagePageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDPage_GenericDesktop), 32);
}

IOReturn VIRTUAL_HID_KEYBOARD_CLASS::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
  *descriptor = IOBufferMemoryDescriptor::withBytes(reportDescriptor_, sizeof(reportDescriptor_), kIODirectionNone);
  return kIOReturnSuccess;
}

OSString* VIRTUAL_HID_KEYBOARD_CLASS::newSerialNumberString() const {
  return OSString::withCString(serialNumberCString());
}

OSNumber* VIRTUAL_HID_KEYBOARD_CLASS::newLocationIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}
