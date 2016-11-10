#include "VirtualHIDPointing.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDPointing, IOHIDDevice);

namespace {
uint8_t reportDescriptor_[] = {
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,        // USAGE (Mouse)
    0xa1, 0x01,        // COLLECTION (Application)
    0x09, 0x02,        //   USAGE (Mouse)
    0xa1, 0x02,        //   COLLECTION (Logical)
    0x09, 0x01,        //     USAGE (Pointer)
    0xa1, 0x00,        //     COLLECTION (Physical)
    /*              */ // ------------------------------ Buttons
    0x05, 0x09,        //       USAGE_PAGE (Button)
    0x19, 0x01,        //       USAGE_MINIMUM (Button 1)
    0x29, 0x20,        //       USAGE_MAXIMUM (Button 32)
    0x15, 0x00,        //       LOGICAL_MINIMUM (0)
    0x25, 0x01,        //       LOGICAL_MAXIMUM (1)
    0x75, 0x01,        //       REPORT_SIZE (1)
    0x95, 0x20,        //       REPORT_COUNT (32 Buttons)
    0x81, 0x02,        //       INPUT (Data,Var,Abs)
    /*              */ // ------------------------------ X,Y position
    0x05, 0x01,        //       USAGE_PAGE (Generic Desktop)
    0x09, 0x30,        //       USAGE (X)
    0x09, 0x31,        //       USAGE (Y)
    0x15, 0x81,        //       LOGICAL_MINIMUM (-127)
    0x25, 0x7f,        //       LOGICAL_MAXIMUM (127)
    0x75, 0x08,        //       REPORT_SIZE (8)
    0x95, 0x02,        //       REPORT_COUNT (2)
    0x81, 0x06,        //       INPUT (Data,Var,Rel)
    0xa1, 0x02,        //       COLLECTION (Logical)
    /*              */ // ------------------------------ Vertical wheel res multiplier
    0x09, 0x48,        //         USAGE (Resolution Multiplier)
    0x15, 0x00,        //         LOGICAL_MINIMUM (0)
    0x25, 0x01,        //         LOGICAL_MAXIMUM (1)
    0x35, 0x01,        //         PHYSICAL_MINIMUM (1)
    0x45, 0x04,        //         PHYSICAL_MAXIMUM (4)
    0x75, 0x02,        //         REPORT_SIZE (2)
    0x95, 0x01,        //         REPORT_COUNT (1)
    0xa4,              //         PUSH
    0xb1, 0x02,        //         FEATURE (Data,Var,Abs)
    /*              */ // ------------------------------ Vertical wheel
    0x09, 0x38,        //         USAGE (Wheel)
    0x15, 0x81,        //         LOGICAL_MINIMUM (-127)
    0x25, 0x7f,        //         LOGICAL_MAXIMUM (127)
    0x35, 0x00,        //         PHYSICAL_MINIMUM (0)        - reset physical
    0x45, 0x00,        //         PHYSICAL_MAXIMUM (0)
    0x75, 0x08,        //         REPORT_SIZE (8)
    0x81, 0x06,        //         INPUT (Data,Var,Rel)
    0xc0,              //       END_COLLECTION
    0xa1, 0x02,        //       COLLECTION (Logical)
    /*              */ // ------------------------------ Horizontal wheel res multiplier
    0x09, 0x48,        //         USAGE (Resolution Multiplier)
    0xb4,              //         POP
    0xb1, 0x02,        //         FEATURE (Data,Var,Abs)
    /*              */ // ------------------------------ Padding for Feature report
    0x35, 0x00,        //         PHYSICAL_MINIMUM (0)        - reset physical
    0x45, 0x00,        //         PHYSICAL_MAXIMUM (0)
    0x75, 0x04,        //         REPORT_SIZE (4)
    0xb1, 0x03,        //         FEATURE (Cnst,Var,Abs)
    /*              */ // ------------------------------ Horizontal wheel
    0x05, 0x0c,        //         USAGE_PAGE (Consumer Devices)
    0x0a, 0x38, 0x02,  //         USAGE (AC Pan)
    0x15, 0x81,        //         LOGICAL_MINIMUM (-127)
    0x25, 0x7f,        //         LOGICAL_MAXIMUM (127)
    0x75, 0x08,        //         REPORT_SIZE (8)
    0x81, 0x06,        //         INPUT (Data,Var,Rel)
    0xc0,              //       END_COLLECTION
    0xc0,              //     END_COLLECTION
    0xc0,              //   END_COLLECTION
    0xc0               // END_COLLECTION};
};
}

bool org_pqrs_driver_VirtualHIDPointing::start(IOService* provider) {
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
    OSNumber* usage = OSNumber::withNumber(kHIDUsage_GD_Mouse, 32);
    if (usage) {
      setProperty(kIOHIDDeviceUsageKey, usage);
      usage->release();
    }
  }

  // http://lists.apple.com/archives/usb/2005/Mar/msg00122.html
  setProperty("HIDDefaultBehavior", "Pointing");

  if (!super::start(provider)) {
    return false;
  }

  return true;
}

OSString* org_pqrs_driver_VirtualHIDPointing::newManufacturerString() const {
  return OSString::withCString("pqrs.org");
}

OSString* org_pqrs_driver_VirtualHIDPointing::newProductString() const {
  return OSString::withCString("pqrs.org VirtualHIDPointing");
}

OSNumber* org_pqrs_driver_VirtualHIDPointing::newVendorIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDPointing::newProductIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDPointing::newPrimaryUsageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDPage_GenericDesktop), 32);
}

OSNumber* org_pqrs_driver_VirtualHIDPointing::newPrimaryUsagePageNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(kHIDUsage_GD_Mouse), 32);
}

IOReturn org_pqrs_driver_VirtualHIDPointing::newReportDescriptor(IOMemoryDescriptor** descriptor) const {
  *descriptor = IOBufferMemoryDescriptor::withBytes(reportDescriptor_, sizeof(reportDescriptor_), kIODirectionNone);
  return kIOReturnSuccess;
}

OSString* org_pqrs_driver_VirtualHIDPointing::newSerialNumberString() const {
  return OSString::withCString("org.pqrs.driver.VirtualHIDPointing");
}

OSNumber* org_pqrs_driver_VirtualHIDPointing::newLocationIDNumber() const {
  return OSNumber::withNumber(static_cast<uint32_t>(0), 32);
}
