#include "UserClient.hpp"

#define super IOUserClient

OSDefineMetaClassAndStructors(VIRTUAL_HID_ROOT_USERCLIENT_CLASS, super);

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
        if (!POINTER->attach(this)) {                                                             \
          goto error;                                                                             \
        }                                                                                         \
                                                                                                  \
        if (!POINTER->start(this)) {                                                              \
          POINTER->detach(this);                                                                  \
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

IOExternalMethodDispatch VIRTUAL_HID_ROOT_USERCLIENT_CLASS::methods_[static_cast<size_t>(pqrs::karabiner_virtual_hid_device::user_client_method::end_)] = {
    // ----------------------------------------
    // VirtualHIDKeyboard

    {
        // initialize_virtual_hid_keyboard
        reinterpret_cast<IOExternalMethodAction>(&staticInitializeVirtualHIDKeyboardCallback), // Method pointer.
        0,                                                                                     // No scalar input value.
        sizeof(pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization),       // One struct input value.
        0,                                                                                     // No scalar output value.
        0                                                                                      // No struct output value.
    },
    {
        // terminate_virtual_hid_keyboard
        reinterpret_cast<IOExternalMethodAction>(&staticTerminateVirtualHIDKeyboardCallback), // Method pointer.
        0,                                                                                    // No scalar input value.
        0,                                                                                    // No struct input value.
        0,                                                                                    // No scalar output value.
        0                                                                                     // No struct output value.
    },
    {
        // dispatch_keyboard_event
        reinterpret_cast<IOExternalMethodAction>(&staticDispatchKeyboardEventCallback), // Method pointer.
        0,                                                                              // No scalar input value.
        sizeof(pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event),  // One struct input value.
        0,                                                                              // No scalar output value.
        0                                                                               // No struct output value.
    },
    {
        // post_keyboard_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticPostKeyboardInputReportCallback), // Method pointer.
        0,                                                                                // No scalar input value.
        sizeof(pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input),           // One struct input value.
        0,                                                                                // No scalar output value.
        0                                                                                 // No struct output value.
    },
    {
        // reset_virtual_hid_keyboard
        reinterpret_cast<IOExternalMethodAction>(&staticResetVirtualHIDKeyboardCallback), // Method pointer.
        0,                                                                                // No scalar input value.
        0,                                                                                // No struct input value.
        0,                                                                                // No scalar output value.
        0                                                                                 // No struct output value.
    },

    // ----------------------------------------
    // VirtualHIDPointing

    {
        // initialize_virtual_hid_pointing
        reinterpret_cast<IOExternalMethodAction>(&staticInitializeVirtualHIDPointingCallback), // Method pointer.
        0,                                                                                     // No scalar input value.
        0,                                                                                     // No struct input value.
        0,                                                                                     // No scalar output value.
        0                                                                                      // No struct output value.
    },
    {
        // terminate_virtual_hid_pointing
        reinterpret_cast<IOExternalMethodAction>(&staticTerminateVirtualHIDPointingCallback), // Method pointer.
        0,                                                                                    // No scalar input value.
        0,                                                                                    // No struct input value.
        0,                                                                                    // No scalar output value.
        0                                                                                     // No struct output value.
    },
    {
        // post_pointing_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticPostPointingInputReportCallback), // Method pointer.
        0,                                                                                // No scalar input value.
        sizeof(pqrs::karabiner_virtual_hid_device::hid_report::pointing_input),           // One struct input value.
        0,                                                                                // No scalar output value.
        0                                                                                 // No struct output value.
    },
    {
        // reset_virtual_hid_pointing
        reinterpret_cast<IOExternalMethodAction>(&staticResetVirtualHIDPointingCallback), // Method pointer.
        0,                                                                                // No scalar input value.
        0,                                                                                // No struct input value.
        0,                                                                                // No scalar output value.
        0                                                                                 // No struct output value.
    },
};

bool VIRTUAL_HID_ROOT_USERCLIENT_CLASS::initWithTask(task_t owningTask,
                                                     void* securityToken,
                                                     UInt32 type) {
  IOLog(VIRTUAL_HID_ROOT_USERCLIENT_CLASS_STRING "::initWithTask\n");

  if (clientHasPrivilege(owningTask, kIOClientPrivilegeAdministrator) != KERN_SUCCESS) {
    IOLog("%s Error: clientHasPrivilege failed.\n", __PRETTY_FUNCTION__);
    return false;
  }

  if (!super::initWithTask(owningTask, securityToken, type)) {
    IOLog("%s Error: initWithTask failed.\n", __PRETTY_FUNCTION__);
    return false;
  }

  hidInterfaceDetector_.setIsTargetServiceCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS::isTargetHIDInterface, this);
  hidInterfaceDetector_.setNotifier("IOHIDInterface");
  kernelMajorReleaseVersion_ = KernelVersion::getMajorReleaseVersion();
  virtualHIDKeyboard_ = nullptr;
  virtualHIDPointing_ = nullptr;
  virtualHIDEventService_ = nullptr;

  return true;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::clientClose(void) {
  IOLog(VIRTUAL_HID_ROOT_USERCLIENT_CLASS_STRING "::clientClose\n");

  // clear input events.
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    postKeyboardInputReportCallback(report);
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
    postPointingInputReportCallback(report);
  }

  terminateVirtualHIDEventService();
  TERMINATE_VIRTUAL_DEVICE(virtualHIDKeyboard_);
  TERMINATE_VIRTUAL_DEVICE(virtualHIDPointing_);

  hidInterfaceDetector_.unsetNotifier();
  hidInterfaceDetector_.unsetIsTargetServiceCallback();

  if (!terminate()) {
    IOLog("%s Error: terminate failed.\n", __PRETTY_FUNCTION__);
  }

  // DON'T call super::clientClose, which just returns kIOReturnUnsupported.
  return kIOReturnSuccess;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::externalMethod(uint32_t selector,
                                                           IOExternalMethodArguments* arguments,
                                                           IOExternalMethodDispatch* dispatch,
                                                           OSObject* target,
                                                           void* reference) {
  if (selector >= static_cast<uint32_t>(pqrs::karabiner_virtual_hid_device::user_client_method::end_)) {
    return kIOReturnUnsupported;
  }

  dispatch = &(methods_[selector]);
  if (!target) {
    target = this;
  }

  return super::externalMethod(selector, arguments, dispatch, target, reference);
}

#pragma mark - initialize_virtual_hid_keyboard

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticInitializeVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                       void* reference,
                                                                                       IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto properties = static_cast<const pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization*>(arguments->structureInput)) {
    return target->initializeVirtualHIDKeyboardCallback(*properties);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::initializeVirtualHIDKeyboardCallback(const pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization& properties) {
  terminateVirtualHIDKeyboardCallback();

  virtualHIDKeyboardType_ = properties.keyboard_type;
  capsLockDelayMilliseconds_ = properties.caps_lock_delay_milliseconds;

  CREATE_VIRTUAL_DEVICE(VIRTUAL_HID_KEYBOARD_CLASS, virtualHIDKeyboard_);
  return virtualHIDKeyboard_ ? kIOReturnSuccess : kIOReturnError;
}

#pragma mark - initialize_virtual_hid_pointing

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticInitializeVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                       void* reference,
                                                                                       IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }

  return target->initializeVirtualHIDPointingCallback();
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::initializeVirtualHIDPointingCallback(void) {
  CREATE_VIRTUAL_DEVICE(VIRTUAL_HID_POINTING_CLASS, virtualHIDPointing_);
  return virtualHIDPointing_ ? kIOReturnSuccess : kIOReturnError;
}

#pragma mark - terminate_virtual_hid_keyboard

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticTerminateVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                      void* reference,
                                                                                      IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }

  return target->terminateVirtualHIDKeyboardCallback();
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::terminateVirtualHIDKeyboardCallback(void) {
  terminateVirtualHIDEventService();

  TERMINATE_VIRTUAL_DEVICE(virtualHIDKeyboard_);
  return kIOReturnSuccess;
}

#pragma mark - terminate_virtual_hid_pointing

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticTerminateVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                      void* reference,
                                                                                      IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }

  return target->terminateVirtualHIDPointingCallback();
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::terminateVirtualHIDPointingCallback(void) {
  TERMINATE_VIRTUAL_DEVICE(virtualHIDPointing_);
  return kIOReturnSuccess;
}

#pragma mark - post_keyboard_input_report

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticPostKeyboardInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                  void* reference,
                                                                                  IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input*>(arguments->structureInput)) {
    return target->postKeyboardInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::postKeyboardInputReportCallback(const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input& input) {
  IOReturn result = kIOReturnError;

  if (virtualHIDKeyboard_) {
    if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
      result = virtualHIDKeyboard_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
      report->release();
    }
  }

  return result;
}

#pragma mark - post_pointing_input_report

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticPostPointingInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                  void* reference,
                                                                                  IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input*>(arguments->structureInput)) {
    return target->postPointingInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::postPointingInputReportCallback(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& input) {
  IOReturn result = kIOReturnError;

  if (virtualHIDPointing_) {
    if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
      result = virtualHIDPointing_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
      report->release();
    }
  }

  return result;
}

#pragma mark - reset_virtual_hid_keyboard

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticResetVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                  void* reference,
                                                                                  IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }

  return target->resetVirtualHIDKeyboardCallback();
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::resetVirtualHIDKeyboardCallback(void) {
  if (!virtualHIDKeyboard_) {
    return kIOReturnSuccess;
  }

  bool result = kIOReturnSuccess;

  // reset for dispatch_keyboard_event
  if (virtualHIDEventService_) {
    virtualHIDEventService_->dispatchKeyUpAllPressedKeys();
  }

  // reset for post_keyboard_input_report
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    auto kr = postKeyboardInputReportCallback(report);
    if (kr != kIOReturnSuccess) {
      result = kIOReturnError;
    }
  }

  return result;
}

#pragma mark - reset_virtual_hid_pointing

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticResetVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                  void* reference,
                                                                                  IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }

  return target->resetVirtualHIDPointingCallback();
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::resetVirtualHIDPointingCallback(void) {
  if (!virtualHIDPointing_) {
    return kIOReturnSuccess;
  }

  pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
  return postPointingInputReportCallback(report);
}

#pragma mark - dispatch_keyboard_event

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticDispatchKeyboardEventCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                void* reference,
                                                                                IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto keyboard_event = static_cast<const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event*>(arguments->structureInput)) {
    return target->dispatchKeyboardEventCallback(*keyboard_event);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::dispatchKeyboardEventCallback(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event) {
  if (virtualHIDEventService_) {
    virtualHIDEventService_->dispatchKeyboardEvent(keyboard_event.usage_page, keyboard_event.usage, keyboard_event.value);
    return kIOReturnSuccess;
  }

  return kIOReturnError;
}

#pragma mark - other methods

bool VIRTUAL_HID_ROOT_USERCLIENT_CLASS::isTargetHIDInterface(IOService* service, IOService* refCon) {
  if (auto self = OSDynamicCast(VIRTUAL_HID_ROOT_USERCLIENT_CLASS, refCon)) {
    if (auto interface = OSDynamicCast(IOHIDInterface, service)) {
      if (auto serialNumber = interface->getSerialNumber()) {
        if (serialNumber->isEqualTo(VIRTUAL_HID_KEYBOARD_CLASS::serialNumberCString())) {
          if (self->initializeVirtualHIDEventService(interface)) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

bool VIRTUAL_HID_ROOT_USERCLIENT_CLASS::initializeVirtualHIDEventService(IOHIDInterface* hidInterface) {
  terminateVirtualHIDEventService();

  if (hidInterface) {
    virtualHIDEventService_ = new VIRTUAL_HID_EVENT_SERVICE_CLASS();
    if (virtualHIDEventService_) {
      bool initialized = false;

      if (auto init_properties = OSDictionary::withCapacity(2)) {
        if (auto event_service_properties = OSDictionary::withCapacity(1)) {
          if (auto number = OSNumber::withNumber(static_cast<uint32_t>(virtualHIDKeyboardType_), 32)) {
            init_properties->setObject(kIOHIDAltHandlerIdKey, number);

            number->release();
          }
          if (auto number = OSNumber::withNumber(static_cast<uint64_t>(capsLockDelayMilliseconds_), 64)) {
            auto key = kIOHIDKeyboardCapsLockDelay;
            init_properties->setObject(key, number);          // for macOS 10.11
            event_service_properties->setObject(key, number); // for macOS 10.12

            number->release();
          }

          initialized = virtualHIDEventService_->init(init_properties);
          virtualHIDEventService_->setProperties(event_service_properties);

          event_service_properties->release();
        }

        init_properties->release();
      }

      if (initialized) {
        if (virtualHIDEventService_->attach(hidInterface)) {
          if (virtualHIDEventService_->start(hidInterface)) {
            IOLog(VIRTUAL_HID_ROOT_CLASS_STRING "::virtualHIDEventService_ is started\n");
            return true;
          }

          // Error handling
          virtualHIDEventService_->detach(hidInterface);
        }
      }
      // Error handling
      virtualHIDEventService_->release();
      virtualHIDEventService_ = nullptr;
    }
  }

  return false;
}

void VIRTUAL_HID_ROOT_USERCLIENT_CLASS::terminateVirtualHIDEventService(void) {
  if (virtualHIDEventService_) {
    virtualHIDEventService_->dispatchKeyUpAllPressedKeys();

    virtualHIDEventService_->terminate(kIOServiceSynchronous);
    virtualHIDEventService_->release();
    virtualHIDEventService_ = nullptr;
  }
}
