#include "VirtualHIDManager.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

bool org_pqrs_driver_VirtualHIDManager::init(OSDictionary* dict) {
  if (!super::init(dict)) {
    return false;
  }

  virtualHIDPointingDetector_ = new ServiceDetector();

  if (auto serialNumber = OSString::withCString("org.pqrs.driver.VirtualHIDManager")) {
    setProperty(kIOHIDSerialNumberKey, serialNumber);
    serialNumber->release();
  }

  return true;
}

void org_pqrs_driver_VirtualHIDManager::free(void) {
  if (virtualHIDPointingDetector_) {
    delete virtualHIDPointingDetector_;
    virtualHIDPointingDetector_ = nullptr;
  }

  super::free();
}

bool org_pqrs_driver_VirtualHIDManager::start(IOService* provider) {
  if (!super::start(provider)) {
    return false;
  }

  if (virtualHIDPointingDetector_) {
    virtualHIDPointingDetector_->setNotifier("org_pqrs_driver_VirtualHIDPointing");
  }

  // Publish ourselves so clients can find us
  //
  // Note:
  // IOHIDDevice (VirtualHIDPointing, etc) cannot create custom UserClient directly.
  // Therefore, we provide IOHIDManager for UserClient.
  registerService();

  return true;
}

void org_pqrs_driver_VirtualHIDManager::stop(IOService* provider) {
  if (virtualHIDPointingDetector_) {
    virtualHIDPointingDetector_->unsetNotifier();
  }
}
