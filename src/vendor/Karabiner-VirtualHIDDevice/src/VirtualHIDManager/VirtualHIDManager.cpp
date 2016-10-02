#include "VirtualHIDManager.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

bool org_pqrs_driver_VirtualHIDManager::init(OSDictionary* dict) {
  IOLog("org_pqrs_driver_VirtualHIDManager::init\n");

  if (!super::init(dict)) {
    return false;
  }

  attachedClientCount_ = 0;
  virtualHIDPointing_ = nullptr;

  if (auto serialNumber = OSString::withCString("org.pqrs.driver.VirtualHIDManager")) {
    setProperty(kIOHIDSerialNumberKey, serialNumber);
    serialNumber->release();
  }

  return true;
}

void org_pqrs_driver_VirtualHIDManager::free(void) {
  IOLog("org_pqrs_driver_VirtualHIDManager::free\n");

  super::free();
}

bool org_pqrs_driver_VirtualHIDManager::start(IOService* provider) {
  IOLog("org_pqrs_driver_VirtualHIDManager::start\n");

  if (!super::start(provider)) {
    return false;
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
  IOLog("org_pqrs_driver_VirtualHIDManager::stop\n");

  terminateVirtualHIDPointing();

  super::stop(provider);
}

void org_pqrs_driver_VirtualHIDManager::attachClient(void) {
  ++attachedClientCount_;

  IOLog("org_pqrs_driver_VirtualHIDManager::attachClient attachedClientCount_ = %d\n", static_cast<int>(attachedClientCount_));

  createVirtualHIDPointing();
}

void org_pqrs_driver_VirtualHIDManager::detachClient(void) {
  if (attachedClientCount_ > 0) {
    --attachedClientCount_;
  }

  IOLog("org_pqrs_driver_VirtualHIDManager::detachClient attachedClientCount_ = %d\n", static_cast<int>(attachedClientCount_));

  if (attachedClientCount_ == 0 && virtualHIDPointing_) {
    terminateVirtualHIDPointing();
  }
}

void org_pqrs_driver_VirtualHIDManager::createVirtualHIDPointing(void) {
  if (virtualHIDPointing_) {
    return;
  }

  // See IOHIDResourceDeviceUserClient::createAndStartDevice
  virtualHIDPointing_ = OSTypeAlloc(org_pqrs_driver_VirtualHIDPointing);
  if (!virtualHIDPointing_) {
    return;
  }

  if (!virtualHIDPointing_->init(nullptr)) {
    goto error;
  }

  if (!virtualHIDPointing_->attach(this)) {
    goto error;
  }

  if (!virtualHIDPointing_->start(this)) {
    virtualHIDPointing_->detach(this);
    goto error;
  }

  return;

error:
  if (virtualHIDPointing_) {
    virtualHIDPointing_->release();
    virtualHIDPointing_ = nullptr;
  }
}

void org_pqrs_driver_VirtualHIDManager::terminateVirtualHIDPointing(void) {
  if (!virtualHIDPointing_) {
    return;
  }

  virtualHIDPointing_->terminate();
  virtualHIDPointing_->release();
  virtualHIDPointing_ = nullptr;
}

IOReturn org_pqrs_driver_VirtualHIDManager::handleHIDPointingReport(IOMemoryDescriptor* report) {
  if (!virtualHIDPointing_) {
    return kIOReturnError;
  }

  return virtualHIDPointing_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
}
