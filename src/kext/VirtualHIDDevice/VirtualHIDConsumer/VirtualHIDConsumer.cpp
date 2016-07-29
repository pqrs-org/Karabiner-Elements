#include "VirtualHIDConsumer.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDConsumer, IOHIDDevice);

namespace {
uint8_t reportDescriptor_[] = {
    0x05, 0x0C, // Usage Page (Consumer)
    0x09, 0x01, // Usage 1 (0x1)
    0xA1, 0x01, // Collection (Application)
    0x05, 0x0C, //   Usage Page (Consumer)
    0x09, 0xB5, //   Usage 181 (0xb5)
    0x09, 0xB6, //   Usage 182 (0xb6)
    0x09, 0xCD, //   Usage 205 (0xcd)
    0x15, 0x00, //   Logical Minimum......... (0)
    0x25, 0x01, //   Logical Maximum......... (1)
    0x75, 0x01, //   Report Size............. (1)
    0x95, 0x04, //   Report Count............ (3)
    0x81, 0x02, //   Input...................(Data, Variable, Absolute)
    0x75, 0x04, //   Report Size............. (5)
    0x95, 0x01, //   Report Count............ (1)
    0x81, 0x01, //   Input...................(Constant)
    0xC0,       // End Collection
};
}

bool org_pqrs_driver_VirtualHIDConsumer::start(IOService* provider) {
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
  setProperty("HIDDefaultBehavior", "Consumer");

  if (!super::start(provider)) {
    return false;
  }

  return true;
}

OSString* org_pqrs_driver_VirtualHIDConsumer::newManufacturerString() const {
  return OSString::withCString("pqrs.org");
}

OSString* org_pqrs_driver_VirtualHIDConsumer::newProductString() const {
  return OSString::withCString("VirtualHIDConsumer");
}

OSNumber* org_pqrs_driver_VirtualHIDConsumer::newVendorIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDConsumer::newProductIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDConsumer::newPrimaryUsageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDPage_GenericDesktop), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDConsumer::newPrimaryUsagePageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDUsage_GD_Keyboard), 32);
}

IOReturn org_pqrs_driver_VirtualHIDConsumer::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
  *descriptor = IOBufferMemoryDescriptor::withBytes(reportDescriptor_, sizeof(reportDescriptor_), kIODirectionNone);
  return kIOReturnSuccess;
}

OSString* org_pqrs_driver_VirtualHIDConsumer::newSerialNumberString() const {
  return OSString::withCString("org.pqrs.driver.VirtualHIDConsumer");
}

OSNumber* org_pqrs_driver_VirtualHIDConsumer::newLocationIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}
