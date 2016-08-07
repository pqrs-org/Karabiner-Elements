#include "VirtualHIDManager.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

bool org_pqrs_driver_VirtualHIDManager::init(OSDictionary* dict) {
  if (!super::init(dict)) {
    return false;
  }

  if (auto serialNumber = OSString::withCString("org.pqrs.driver.VirtualHIDManager")) {
    setProperty(kIOHIDSerialNumberKey, serialNumber);
    serialNumber->release();
  }

  return true;
}

bool org_pqrs_driver_VirtualHIDManager::start(IOService* provider) {
  if (!super::start(provider)) {
    return false;
  }

  virtualHIDKeyboardDetector_.setNotifier("org_pqrs_driver_VirtualHIDKeyboard");
  virtualHIDPointingDetector_.setNotifier("org_pqrs_driver_VirtualHIDPointing");

  // Publish ourselves so clients can find us
  //
  // Note:
  // IOHIDDevice (VirtualHIDKeyboard, etc) cannot create custom UserClient directly.
  // Therefore, we provide IOHIDManager for UserClient.
  registerService();

  return true;
}

void org_pqrs_driver_VirtualHIDManager::stop(IOService* provider) {
  virtualHIDKeyboardDetector_.unsetNotifier();
  virtualHIDPointingDetector_.unsetNotifier();
}
