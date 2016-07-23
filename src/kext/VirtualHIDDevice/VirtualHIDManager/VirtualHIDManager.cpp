#include "VirtualHIDManager.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

bool org_pqrs_driver_VirtualHIDManager::start(IOService* provider) {
  virtualHIDKeyboardMatchedNotifier_ = nullptr;
  virtualHIDKeyboardTerminatedNotifier_ = nullptr;
  virtualHIDKeyboard_ = nullptr;

  if (!super::start(provider)) {
    return false;
  }

  virtualHIDKeyboardMatchedNotifier_ = addMatchingNotification(gIOMatchedNotification,
                                                               serviceMatching("org_pqrs_driver_VirtualHIDKeyboard"),
                                                               virtualHIDKeyboardMatchedCallback,
                                                               this, nullptr, 0);

  virtualHIDKeyboardTerminatedNotifier_ = addMatchingNotification(gIOTerminatedNotification,
                                                                  serviceMatching("org_pqrs_driver_VirtualHIDKeyboard"),
                                                                  virtualHIDKeyboardTerminatedCallback,
                                                                  this, nullptr, 0);

  // Publish ourselves so clients can find us
  //
  // Note:
  // IOHIDDevice (VirtualHIDKeyboard, etc) cannot create custom UserClient directly.
  // Therefore, we provide IOHIDManager for UserClient.
  registerService();

  return true;
}

void org_pqrs_driver_VirtualHIDManager::stop(IOService* provider) {
  if (virtualHIDKeyboardMatchedNotifier_) {
    virtualHIDKeyboardMatchedNotifier_->remove();
    virtualHIDKeyboardMatchedNotifier_ = nullptr;
  }

  if (virtualHIDKeyboardTerminatedNotifier_) {
    virtualHIDKeyboardTerminatedNotifier_->remove();
    virtualHIDKeyboardTerminatedNotifier_ = nullptr;
  }
}

bool org_pqrs_driver_VirtualHIDManager::virtualHIDKeyboardMatchedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
  auto self = static_cast<org_pqrs_driver_VirtualHIDManager*>(target);
  if (self) {
    self->virtualHIDKeyboard_ = OSDynamicCast(org_pqrs_driver_VirtualHIDKeyboard, newService);
  }
  return true;
}

bool org_pqrs_driver_VirtualHIDManager::virtualHIDKeyboardTerminatedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
  auto self = static_cast<org_pqrs_driver_VirtualHIDManager*>(target);
  if (self) {
    self->virtualHIDKeyboard_ = nullptr;
  }
  return true;
}
