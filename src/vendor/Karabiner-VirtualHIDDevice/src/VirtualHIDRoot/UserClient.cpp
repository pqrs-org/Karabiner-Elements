#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/hidsystem/IOHIDSystem.h>
END_IOKIT_INCLUDE;

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

IOExternalMethodDispatch VIRTUAL_HID_ROOT_USERCLIENT_CLASS::methods_[static_cast<size_t>(pqrs::karabiner_virtualhiddevice::user_client_method::end_)] = {
    {
        // initialize_virtual_hid_keyboard
        reinterpret_cast<IOExternalMethodAction>(&staticInitializeVirtualHIDKeyboardCallback), // Method pointer.
        0,                                                                                     // No scalar input value.
        0,                                                                                     // No struct input value.
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
        // post_keyboard_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticPostKeyboardInputReportCallback), // Method pointer.
        0,                                                                                // No scalar input value.
        sizeof(pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input),             // One struct input value.
        0,                                                                                // No scalar output value.
        0                                                                                 // No struct output value.
    },
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
        sizeof(pqrs::karabiner_virtualhiddevice::hid_report::pointing_input),             // One struct input value.
        0,                                                                                // No scalar output value.
        0                                                                                 // No struct output value.
    },
    {
        // post_keyboard_event
        reinterpret_cast<IOExternalMethodAction>(&staticPostKeyboardEventCallback), // Method pointer.
        0,                                                                          // No scalar input value.
        sizeof(pqrs::karabiner_virtualhiddevice::keyboard_event),                   // One struct input value.
        0,                                                                          // No scalar output value.
        0                                                                           // No struct output value.
    },
    {
        // post_keyboard_special_event
        reinterpret_cast<IOExternalMethodAction>(&staticPostKeyboardSpecialEventCallback), // Method pointer.
        0,                                                                                 // No scalar input value.
        sizeof(pqrs::karabiner_virtualhiddevice::keyboard_special_event),                  // One struct input value.
        0,                                                                                 // No scalar output value.
        0                                                                                  // No struct output value.
    },
    {
        // update_event_flags
        reinterpret_cast<IOExternalMethodAction>(&staticUpdateEventFlagsCallback), // Method pointer.
        0,                                                                         // No scalar input value.
        sizeof(uint32_t),                                                          // One struct input value.
        0,                                                                         // No scalar output value.
        0                                                                          // No struct output value.
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

  kernelMajorReleaseVersion_ = KernelVersion::getMajorReleaseVersion();
  virtualHIDKeyboard_ = nullptr;
  virtualHIDPointing_ = nullptr;

  return true;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::clientClose(void) {
  IOLog(VIRTUAL_HID_ROOT_USERCLIENT_CLASS_STRING "::clientClose\n");

  // clear input events.
  {
    pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input report;
    postKeyboardInputReportCallback(report);
  }
  {
    pqrs::karabiner_virtualhiddevice::hid_report::pointing_input report;
    postPointingInputReportCallback(report);
  }

  TERMINATE_VIRTUAL_DEVICE(virtualHIDKeyboard_);
  TERMINATE_VIRTUAL_DEVICE(virtualHIDPointing_);

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
  if (selector >= static_cast<uint32_t>(pqrs::karabiner_virtualhiddevice::user_client_method::end_)) {
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

  return target->initializeVirtualHIDKeyboardCallback();
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::initializeVirtualHIDKeyboardCallback(void) {
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

#pragma mark - keyboard_input_report

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticPostKeyboardInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                  void* reference,
                                                                                  IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input*>(arguments->structureInput)) {
    return target->postKeyboardInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::postKeyboardInputReportCallback(const pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input& input) {
  if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
    IOReturn result = kIOReturnError;
    if (virtualHIDKeyboard_) {
      result = virtualHIDKeyboard_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
    }
    report->release();
    return result;
  }

  return kIOReturnError;
}

#pragma mark - pointing_input_report

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticPostPointingInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                  void* reference,
                                                                                  IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const pqrs::karabiner_virtualhiddevice::hid_report::pointing_input*>(arguments->structureInput)) {
    return target->postPointingInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::postPointingInputReportCallback(const pqrs::karabiner_virtualhiddevice::hid_report::pointing_input& input) {
  if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
    IOReturn result = kIOReturnError;
    if (virtualHIDPointing_) {
      result = virtualHIDPointing_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
    }
    report->release();
    return result;
  }

  return kIOReturnError;
}

#pragma mark - post_keyboard_event

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticPostKeyboardEventCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                            void* reference,
                                                                            IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto keyboard_event = static_cast<const pqrs::karabiner_virtualhiddevice::keyboard_event*>(arguments->structureInput)) {
    return target->postKeyboardEventCallback(*keyboard_event);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::postKeyboardEventCallback(const pqrs::karabiner_virtualhiddevice::keyboard_event& keyboard_event) {
  if (kernelMajorReleaseVersion_ < 16) {
    IOLog(VIRTUAL_HID_ROOT_USERCLIENT_CLASS_STRING "::postKeyboardEventCallback requires macOS 10.12 or later.\n");
    // IOHIDSystem::keyboardEvent is available since macOS Sierra.
    // (This method exists in previous macOS releases, but it causes kernel panic.)
    return kIOReturnUnsupported;
  }

  if (auto hidSystem = IOHIDSystem::instance()) {
    AbsoluteTime ts;
    clock_get_uptime(&ts);

    hidSystem->keyboardEvent(static_cast<uint32_t>(keyboard_event.event_type),
                             keyboard_event.flags,
                             keyboard_event.key,
                             keyboard_event.char_code,
                             keyboard_event.char_set,
                             keyboard_event.orig_char_code,
                             keyboard_event.orig_char_set,
                             keyboard_event.keyboard_type,
                             keyboard_event.repeat,
                             ts);

    return kIOReturnSuccess;
  }

  return kIOReturnError;
}

#pragma mark - post_keyboard_special_event

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticPostKeyboardSpecialEventCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                                   void* reference,
                                                                                   IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto keyboard_special_event = static_cast<const pqrs::karabiner_virtualhiddevice::keyboard_special_event*>(arguments->structureInput)) {
    return target->postKeyboardSpecialEventCallback(*keyboard_special_event);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::postKeyboardSpecialEventCallback(const pqrs::karabiner_virtualhiddevice::keyboard_special_event& keyboard_special_event) {
  if (kernelMajorReleaseVersion_ < 16) {
    IOLog(VIRTUAL_HID_ROOT_USERCLIENT_CLASS_STRING "::postKeyboardSpecialEventCallback requires macOS 10.12 or later.\n");
    // IOHIDSystem::keyboardSpecialEvent is available since macOS Sierra.
    // (This method exists in previous macOS releases, but it causes kernel panic.)
    return kIOReturnUnsupported;
  }

  if (auto hidSystem = IOHIDSystem::instance()) {
    AbsoluteTime ts;
    clock_get_uptime(&ts);

    hidSystem->keyboardSpecialEvent(static_cast<uint32_t>(keyboard_special_event.event_type),
                                    keyboard_special_event.flags,
                                    keyboard_special_event.key,
                                    keyboard_special_event.flavor,
                                    keyboard_special_event.guid,
                                    keyboard_special_event.repeat,
                                    ts);

    return kIOReturnSuccess;
  }

  return kIOReturnError;
}

#pragma mark - update_event_flags

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::staticUpdateEventFlagsCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                                           void* reference,
                                                                           IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto flags = static_cast<const uint32_t*>(arguments->structureInput)) {
    return target->updateEventFlagsCallback(*flags);
  }

  return kIOReturnBadArgument;
}

IOReturn VIRTUAL_HID_ROOT_USERCLIENT_CLASS::updateEventFlagsCallback(const uint32_t& flags) {
  if (kernelMajorReleaseVersion_ < 16) {
    IOLog(VIRTUAL_HID_ROOT_USERCLIENT_CLASS_STRING "::updateEventFlagsCallback requires macOS 10.12 or later.\n");
    // IOHIDSystem::keyboardSpecialEvent is available since macOS Sierra.
    // (This method exists in previous macOS releases, but it causes kernel panic.)
    return kIOReturnUnsupported;
  }

  if (auto hidSystem = IOHIDSystem::instance()) {
    AbsoluteTime ts;
    clock_get_uptime(&ts);

    hidSystem->updateEventFlags(flags);
    return kIOReturnSuccess;
  }

  return kIOReturnError;
}
