#include "VirtualHIDManager.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

bool org_pqrs_driver_VirtualHIDManager::init(OSDictionary* dict) {
  if (!super::init(dict)) {
    return false;
  }

  virtualHIDKeyboardDetector_ = new ServiceDetector();
  virtualHIDPointingDetector_ = new ServiceDetector();

  if (auto serialNumber = OSString::withCString("org.pqrs.driver.VirtualHIDManager")) {
    setProperty(kIOHIDSerialNumberKey, serialNumber);
    serialNumber->release();
  }

  return true;
}

void org_pqrs_driver_VirtualHIDManager::free(void) {
  if (virtualHIDKeyboardDetector_) {
    delete virtualHIDKeyboardDetector_;
    virtualHIDKeyboardDetector_ = nullptr;
  }
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

  if (virtualHIDKeyboardDetector_) {
    virtualHIDKeyboardDetector_->setNotifier("org_pqrs_driver_VirtualHIDKeyboard");
  }
  if (virtualHIDPointingDetector_) {
    virtualHIDPointingDetector_->setNotifier("org_pqrs_driver_VirtualHIDPointing");
  }

  // Publish ourselves so clients can find us
  //
  // Note:
  // IOHIDDevice (VirtualHIDKeyboard, etc) cannot create custom UserClient directly.
  // Therefore, we provide IOHIDManager for UserClient.
  registerService();

  return true;
}

void org_pqrs_driver_VirtualHIDManager::stop(IOService* provider) {
  if (virtualHIDKeyboardDetector_) {
    virtualHIDKeyboardDetector_->unsetNotifier();
  }
  if (virtualHIDPointingDetector_) {
    virtualHIDPointingDetector_->unsetNotifier();
  }
}
