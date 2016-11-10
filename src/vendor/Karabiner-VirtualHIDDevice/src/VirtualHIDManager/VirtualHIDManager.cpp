#include "VirtualHIDManager.hpp"
#include "GlobalLock.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

#define CREATE_VIRTUAL_DEVICE(CLASS, POINTER)                                                     \
  {                                                                                               \
    if (!POINTER) {                                                                               \
      do {                                                                                        \
        /* See IOKit Fundamentals > The Base Classes > Object Creation and Disposal (OSObject) */ \
        POINTER = new CLASS;                                                                      \
        if (!POINTER) {                                                                           \
          return kIOReturnError;                                                                  \
        }                                                                                         \
                                                                                                  \
        if (!POINTER->init(nullptr)) {                                                            \
          goto error;                                                                             \
        }                                                                                         \
                                                                                                  \
        if (!POINTER->attach(provider_)) {                                                        \
          goto error;                                                                             \
        }                                                                                         \
                                                                                                  \
        if (!POINTER->start(provider_)) {                                                         \
          POINTER->detach(provider_);                                                             \
          goto error;                                                                             \
        }                                                                                         \
                                                                                                  \
        /* The virtual device is created! */                                                      \
        break;                                                                                    \
                                                                                                  \
      error:                                                                                      \
        if (POINTER) {                                                                            \
          POINTER->release();                                                                     \
          POINTER = nullptr;                                                                      \
        }                                                                                         \
        return kIOReturnError;                                                                    \
      } while (false);                                                                            \
    }                                                                                             \
  }

#define TERMINATE_VIRTUAL_DEVICE(POINTER)        \
  {                                              \
    if (POINTER) {                               \
      POINTER->terminate(kIOServiceSynchronous); \
      POINTER->release();                        \
      POINTER = nullptr;                         \
    }                                            \
  }

bool org_pqrs_driver_VirtualHIDManager::init(OSDictionary* dict) {
  IOLog("org_pqrs_driver_VirtualHIDManager::init\n");

  if (!super::init(dict)) {
    return false;
  }

  org_pqrs_driver_VirtualHIDManager_GlobalLock::initialize();

  provider_ = nullptr;
  attachedClientCount_ = 0;
  virtualHIDKeyboard_ = nullptr;
  virtualHIDPointing_ = nullptr;

  if (auto serialNumber = OSString::withCString("org.pqrs.driver.VirtualHIDManager")) {
    setProperty(kIOHIDSerialNumberKey, serialNumber);
    serialNumber->release();
  }

  return true;
}

void org_pqrs_driver_VirtualHIDManager::free(void) {
  IOLog("org_pqrs_driver_VirtualHIDManager::free\n");

  org_pqrs_driver_VirtualHIDManager_GlobalLock::terminate();

  super::free();
}

bool org_pqrs_driver_VirtualHIDManager::start(IOService* provider) {
  IOLog("org_pqrs_driver_VirtualHIDManager::start\n");

  if (!super::start(provider)) {
    return false;
  }

  provider_ = provider;

  // Publish ourselves so clients can find us
  //
  // Note:
  // IOHIDDevice (VirtualHIDPointing, etc) cannot create custom UserClient directly.
  // Therefore, we provide IOHIDManager for UserClient.
  registerService();

  return true;
}

void org_pqrs_driver_VirtualHIDManager::stop(IOService* provider) {
  IOLog("org_pqrs_driver_VirtualHIDManager::stop\n");

  TERMINATE_VIRTUAL_DEVICE(virtualHIDKeyboard_);
  TERMINATE_VIRTUAL_DEVICE(virtualHIDPointing_);

  super::stop(provider);
}

void org_pqrs_driver_VirtualHIDManager::attachClient(void) {
  org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock lock;

  ++attachedClientCount_;

  IOLog("org_pqrs_driver_VirtualHIDManager::attachClient attachedClientCount_ = %d\n", static_cast<int>(attachedClientCount_));
}

void org_pqrs_driver_VirtualHIDManager::detachClient(void) {
  org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock lock;

  if (attachedClientCount_ > 0) {
    --attachedClientCount_;
  }

  IOLog("org_pqrs_driver_VirtualHIDManager::detachClient attachedClientCount_ = %d\n", static_cast<int>(attachedClientCount_));

  if (attachedClientCount_ == 0) {
    TERMINATE_VIRTUAL_DEVICE(virtualHIDKeyboard_);
    TERMINATE_VIRTUAL_DEVICE(virtualHIDPointing_);
  }
}

void org_pqrs_driver_VirtualHIDManager::terminateVirtualHIDKeyboard(void) {
  org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock lock;

  TERMINATE_VIRTUAL_DEVICE(virtualHIDKeyboard_);
}

void org_pqrs_driver_VirtualHIDManager::terminateVirtualHIDPointing(void) {
  org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock lock;

  TERMINATE_VIRTUAL_DEVICE(virtualHIDPointing_);
}

IOReturn org_pqrs_driver_VirtualHIDManager::handleHIDKeyboardReport(IOMemoryDescriptor* report) {
  org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock lock;

  CREATE_VIRTUAL_DEVICE(org_pqrs_driver_VirtualHIDKeyboard, virtualHIDKeyboard_);

  if (!virtualHIDKeyboard_) {
    return kIOReturnError;
  }

  return virtualHIDKeyboard_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
}

IOReturn org_pqrs_driver_VirtualHIDManager::handleHIDPointingReport(IOMemoryDescriptor* report) {
  org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock lock;

  CREATE_VIRTUAL_DEVICE(org_pqrs_driver_VirtualHIDPointing, virtualHIDPointing_);

  if (!virtualHIDPointing_) {
    return kIOReturnError;
  }

  return virtualHIDPointing_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
}
