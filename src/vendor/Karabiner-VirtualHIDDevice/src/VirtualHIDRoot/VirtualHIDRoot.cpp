#include "VirtualHIDRoot.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VIRTUAL_HID_ROOT_CLASS, super);

bool VIRTUAL_HID_ROOT_CLASS::start(IOService* provider) {
  IOLog(VIRTUAL_HID_ROOT_CLASS_STRING "::start\n");

  if (!super::start(provider)) {
    return false;
  }

  // Publish ourselves so clients can find us
  //
  // Note:
  // IOHIDDevice (VirtualHIDPointing, etc) cannot create custom UserClient directly.
  // Therefore, we provide UserClient here.
  registerService();

  return true;
}

void VIRTUAL_HID_ROOT_CLASS::stop(IOService* provider) {
  IOLog(VIRTUAL_HID_ROOT_CLASS_STRING "::stop\n");

  super::stop(provider);
}
