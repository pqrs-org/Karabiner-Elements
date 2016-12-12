#include "VirtualHIDEventService.hpp"

#define super IOHIDEventService
OSDefineMetaClassAndStructors(VIRTUAL_HID_EVENT_SERVICE_CLASS, super);

bool VIRTUAL_HID_EVENT_SERVICE_CLASS::handleStart(IOService* provider) {
  reportElements_ = nullptr;

  auto hidInterface = OSDynamicCast(IOHIDInterface, provider);
  if (!hidInterface) {
    IOLog(VIRTUAL_HID_EVENT_SERVICE_CLASS_STRING "::handleStart provider error (provider must be IOHIDInterface).\n");
    return false;
  }
  reportElements_ = hidInterface->createMatchingElements();

  for (size_t i = 0; i < sizeof(pressedKeys_) / sizeof(pressedKeys_[0]); ++i) {
    pressedKeys_[i] = 0;
  }

  return super::handleStart(provider);
}

void VIRTUAL_HID_EVENT_SERVICE_CLASS::handleStop(IOService* provider) {
  super::handleStop(provider);

  if (reportElements_) {
    reportElements_->release();
    reportElements_ = nullptr;
  }
}

void VIRTUAL_HID_EVENT_SERVICE_CLASS::dispatchKeyboardEvent(pqrs::karabiner_virtual_hid_device::usage_page usagePage,
                                                            pqrs::karabiner_virtual_hid_device::usage usage,
                                                            UInt32 value) {
  AbsoluteTime ts;
  clock_get_uptime(&ts);

  // options values:
  // * kHIDDispatchOptionKeyboardNoRepeat
  IOOptionBits options = 0;

  super::dispatchKeyboardEvent(ts, static_cast<UInt32>(usagePage), static_cast<UInt32>(usage), value, options);

  // Register to pressedKeys_
  {
    auto pressedKey = (static_cast<UInt64>(usagePage) << 32) | static_cast<UInt64>(usage);

    for (size_t i = 0; i < sizeof(pressedKeys_) / sizeof(pressedKeys_[0]); ++i) {
      if (pressedKeys_[i] == pressedKey) {
        pressedKeys_[i] = 0;
      }
    }

    if (value) {
      for (size_t i = 0; i < sizeof(pressedKeys_) / sizeof(pressedKeys_[0]); ++i) {
        if (pressedKeys_[i] == 0) {
          pressedKeys_[i] = pressedKey;
          break;
        }
      }
    }
  }
}

void VIRTUAL_HID_EVENT_SERVICE_CLASS::dispatchKeyUpAllPressedKeys(void) {
  for (size_t i = 0; i < sizeof(pressedKeys_) / sizeof(pressedKeys_[0]); ++i) {
    if (pressedKeys_[i] != 0) {
      UInt32 usagePage = static_cast<UInt32>((pressedKeys_[i] >> 32) & 0xffffffff);
      UInt32 usage = static_cast<UInt32>(pressedKeys_[i] & 0xffffffff);
      dispatchKeyboardEvent(pqrs::karabiner_virtual_hid_device::usage_page(usagePage),
                            pqrs::karabiner_virtual_hid_device::usage(usage),
                            0);
      pressedKeys_[i] = 0;
    }
  }
}
