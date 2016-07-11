#include "VirtualHIDKeyboard.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDKeyboard, IOHIDDevice);

namespace {
uint8_t reportDescriptor_[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,       //   Usage Minimum........... (224)
    0x29, 0xE7,       //   Usage Maximum........... (231)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x25, 0x01,       //   Logical Maximum......... (1)
    0x75, 0x01,       //   Report Size............. (1)
    0x95, 0x08,       //   Report Count............ (8)
    0x81, 0x02,       //   Input...................(Data, Variable, Absolute)
    0x95, 0x01,       //   Report Count............ (1)
    0x75, 0x08,       //   Report Size............. (8)
    0x81, 0x01,       //   Input...................(Constant)
    0x95, 0x02,       //   Report Count............ (2)
    0x75, 0x01,       //   Report Size............. (1)
    0x05, 0x08,       //   Usage Page (LED)
    0x19, 0x01,       //   Usage Minimum........... (1)
    0x29, 0x02,       //   Usage Maximum........... (2)
    0x91, 0x02,       //   Output..................(Data, Variable, Absolute)
    0x95, 0x01,       //   Report Count............ (1)
    0x75, 0x06,       //   Report Size............. (6)
    0x91, 0x01,       //   Output..................(Constant)
    0x95, 0x06,       //   Report Count............ (6)
    0x75, 0x08,       //   Report Size............. (8)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xFF, 0x00, //   Logical Maximum......... (255)
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,       //   Usage Minimum........... (0)
    0x29, 0xFF,       //   Usage Maximum........... (-1)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
    0xC0,             // End Collection
};
}

bool org_pqrs_driver_VirtualHIDKeyboard::start(IOService* provider) {
  // set kIOHIDDeviceUsagePageKey
  {
    OSNumber* usagePage = OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
    if (usagePage) {
      setProperty(kIOHIDDeviceUsagePageKey, usagePage);
      usagePage->release();
    }
  }

  // set kIOHIDDeviceUsageKey
  {
    OSNumber* usage = OSNumber::withNumber(kHIDUsage_GD_Keyboard, 32);
    if (usage) {
      setProperty(kIOHIDDeviceUsageKey, usage);
      usage->release();
    }
  }

  // http://lists.apple.com/archives/usb/2005/Mar/msg00122.html
  setProperty("HIDDefaultBehavior", "Keyboard");

  setProperty(kIOHIDVirtualHIDevice, kOSBooleanTrue);

  return super::start(provider);
}

OSString* org_pqrs_driver_VirtualHIDKeyboard::newManufacturerString() const {
  return OSString::withCString("pqrs.org");
}

OSString* org_pqrs_driver_VirtualHIDKeyboard::newProductString() const {
  return OSString::withCString("VirtualHIDKeyboard");
}

OSNumber* org_pqrs_driver_VirtualHIDKeyboard::newVendorIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDKeyboard::newProductIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDKeyboard::newPrimaryUsageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDPage_GenericDesktop), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDKeyboard::newPrimaryUsagePageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDUsage_GD_Keyboard), 32);
}

IOReturn org_pqrs_driver_VirtualHIDKeyboard::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
  *descriptor = IOBufferMemoryDescriptor::withBytes(reportDescriptor_, sizeof(reportDescriptor_), kIODirectionNone);
  return kIOReturnSuccess;
}

OSString* org_pqrs_driver_VirtualHIDKeyboard::newSerialNumberString() const {
  return OSString::withCString("org.pqrs.driver.VirtualHIDKeyboard");
}

OSNumber* org_pqrs_driver_VirtualHIDKeyboard::newLocationIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

IOReturn org_pqrs_driver_VirtualHIDKeyboard::setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) {
  IOReturn r = handleReport(report, reportType, options);
  return r;
}
