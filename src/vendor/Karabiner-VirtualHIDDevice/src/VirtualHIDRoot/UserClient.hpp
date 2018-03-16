#pragma once

#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/hid/IOHIDInterface.h>
#include <IOKit/hidsystem/IOHIDSystem.h>
END_IOKIT_INCLUDE;

#include "VersionSignature.hpp"
#include "VirtualHIDKeyboard.hpp"
#include "VirtualHIDPointing.hpp"
#include "VirtualHIDRoot.hpp"
#include "karabiner_virtual_hid_device.hpp"
#include <sys/sysctl.h>

class VIRTUAL_HID_ROOT_USERCLIENT_CLASS final : public IOUserClient {
  OSDeclareDefaultStructors(VIRTUAL_HID_ROOT_USERCLIENT_CLASS);

public:
  virtual bool initWithTask(task_t owningTask,
                            void* securityToken,
                            UInt32 type) override;

  virtual IOReturn clientClose(void) override;

  virtual IOReturn externalMethod(uint32_t selector,
                                  IOExternalMethodArguments* arguments,
                                  IOExternalMethodDispatch* dispatch = 0,
                                  OSObject* target = 0,
                                  void* reference = 0) override;

private:
  // ----------------------------------------
  // VirtualHIDKeyboard callbacks

  static IOReturn staticInitializeVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                             void* reference,
                                                             IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }
    if (!arguments) {
      return kIOReturnBadArgument;
    }

    if (auto properties = static_cast<const pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization*>(arguments->structureInput)) {
      return target->initializeVirtualHIDKeyboard(*properties);
    }

    return kIOReturnBadArgument;
  }

  static IOReturn staticTerminateVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                            void* reference,
                                                            IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }

    return target->terminateVirtualHIDKeyboard();
  }

  static IOReturn staticIsVirtualHIDKeyboardReady(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                  void* reference,
                                                  IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }

    if (!arguments) {
      return kIOReturnBadArgument;
    }

    if (auto ready = static_cast<bool*>(arguments->structureOutput)) {
      *ready = true;
      if (!target->virtualHIDKeyboard_) {
        *ready = false;
      }
      return kIOReturnSuccess;
    }

    return kIOReturnBadArgument;
  }

  static IOReturn staticPostKeyboardInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }
    if (!arguments) {
      return kIOReturnBadArgument;
    }

    return target->postKeyboardInputReport(arguments->structureInput, arguments->structureInputSize);
  }

  static IOReturn staticResetVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }

    return target->resetVirtualHIDKeyboard();
  }

  // ----------------------------------------
  // VirtualHIDKeyboard methods

  IOReturn initializeVirtualHIDKeyboard(const pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization& properties);
  IOReturn terminateVirtualHIDKeyboard(void);

  IOReturn postKeyboardInputReport(const void* report, uint32_t reportSize) const {
    IOReturn result = kIOReturnError;

    if (!report || reportSize == 0) {
      return kIOReturnBadArgument;
    }

    if (virtualHIDKeyboard_) {
      if (auto buffer = IOBufferMemoryDescriptor::withBytes(report, reportSize, kIODirectionNone)) {
        result = virtualHIDKeyboard_->handleReport(buffer, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
        buffer->release();
      }
    }

    return result;
  }

  IOReturn resetVirtualHIDKeyboard(void) const {
    bool result = kIOReturnSuccess;

    if (virtualHIDKeyboard_) {
      {
        pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
        auto kr = postKeyboardInputReport(&report, sizeof(report));
        if (kr != kIOReturnSuccess) {
          result = kIOReturnError;
        }
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::consumer_input report;
        auto kr = postKeyboardInputReport(&report, sizeof(report));
        if (kr != kIOReturnSuccess) {
          result = kIOReturnError;
        }
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input report;
        auto kr = postKeyboardInputReport(&report, sizeof(report));
        if (kr != kIOReturnSuccess) {
          result = kIOReturnError;
        }
      }
      {
        pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input report;
        auto kr = postKeyboardInputReport(&report, sizeof(report));
        if (kr != kIOReturnSuccess) {
          result = kIOReturnError;
        }
      }
    }

    return result;
  }

  // ----------------------------------------
  // VirtualHIDPointing callbacks

  static IOReturn staticInitializeVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                             void* reference,
                                                             IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }

    return target->initializeVirtualHIDPointing();
  }

  static IOReturn staticTerminateVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                            void* reference,
                                                            IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }

    return target->terminateVirtualHIDPointing();
  }

  static IOReturn staticPostPointingInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }
    if (!arguments) {
      return kIOReturnBadArgument;
    }

    if (auto input = static_cast<const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input*>(arguments->structureInput)) {
      return target->postPointingInputReport(*input);
    }

    return kIOReturnBadArgument;
  }

  static IOReturn staticResetVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments) {
    if (!target) {
      return kIOReturnBadArgument;
    }

    return target->resetVirtualHIDPointing();
  }

  // ----------------------------------------
  // VirtualHIDPointing methods

  IOReturn initializeVirtualHIDPointing(void);
  IOReturn terminateVirtualHIDPointing(void);

  IOReturn postPointingInputReport(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& input) const {
    IOReturn result = kIOReturnError;

    if (virtualHIDPointing_) {
      if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
        result = virtualHIDPointing_->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
        report->release();
      }
    }

    return result;
  }

  IOReturn resetVirtualHIDPointing(void) const {
    bool result = kIOReturnSuccess;

    if (virtualHIDPointing_) {
      {
        pqrs::karabiner_virtual_hid_device::hid_report::pointing_input report;
        auto kr = postPointingInputReport(report);
        if (kr != kIOReturnSuccess) {
          result = kIOReturnError;
        }
      }
    }

    return result;
  }

  // ----------------------------------------
  static IOExternalMethodDispatch methods_[static_cast<size_t>(pqrs::karabiner_virtual_hid_device::user_client_method::end_)];
  VIRTUAL_HID_KEYBOARD_CLASS* virtualHIDKeyboard_;
  VIRTUAL_HID_POINTING_CLASS* virtualHIDPointing_;
};
