#include "VirtualHIDManager.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager, IOService);

bool org_pqrs_driver_VirtualHIDManager::start(IOService* provider) {
  if (!super::start(provider)) {
    return false;
  }

  // Publish ourselves so clients can find us
  registerService();

  return true;
}
