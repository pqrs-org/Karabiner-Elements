#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
END_IOKIT_INCLUDE;

#include "UserClient.hpp"

#define super IOUserClient

OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager_UserClient, IOUserClient);

IOExternalMethodDispatch org_pqrs_driver_VirtualHIDManager_UserClient::methods_[static_cast<size_t>(pqrs::karabiner_virtualhiddevice::user_client_method::end_)] = {
    {
        // terminate_virtual_hid_keyboard
        reinterpret_cast<IOExternalMethodAction>(&staticTerminateVirtualHIDKeyboardCallback), // Method pointer.
        0,                                                                                    // No scalar input value.
        0,                                                                                    // No struct input value.
        0,                                                                                    // No scalar output value.
        0                                                                                     // No struct output value.
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
        // keyboard_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticKeyboardInputReportCallback), // Method pointer.
        0,                                                                            // No scalar input value.
        sizeof(pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input),         // One struct input value.
        0,                                                                            // No scalar output value.
        0                                                                             // No struct output value.
    },
    {
        // pointing_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticPointingInputReportCallback), // Method pointer.
        0,                                                                            // No scalar input value.
        sizeof(pqrs::karabiner_virtualhiddevice::hid_report::pointing_input),         // One struct input value.
        0,                                                                            // No scalar output value.
        0                                                                             // No struct output value.
    },
};

bool org_pqrs_driver_VirtualHIDManager_UserClient::initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
  IOLog("org_pqrs_driver_VirtualHIDManager_UserClient::initWithTask\n");

  if (clientHasPrivilege(owningTask, kIOClientPrivilegeAdministrator) != KERN_SUCCESS) {
    IOLog("%s Error: clientHasPrivilege failed.\n", __PRETTY_FUNCTION__);
    return false;
  }

  if (!super::initWithTask(owningTask, securityToken, type)) {
    IOLog("%s Error: initWithTask failed.\n", __PRETTY_FUNCTION__);
    return false;
  }

  owner_ = owningTask;
  provider_ = nullptr;

  return true;
}

bool org_pqrs_driver_VirtualHIDManager_UserClient::start(IOService* provider) {
  IOLog("org_pqrs_driver_VirtualHIDManager_UserClient::start\n");

  provider_ = OSDynamicCast(org_pqrs_driver_VirtualHIDManager, provider);
  if (!provider_) {
    IOLog("%s Error: provider_ == nullptr\n", __PRETTY_FUNCTION__);
    return false;
  }

  if (!super::start(provider)) {
    return false;
  }

  provider_->attachClient();

  return true;
}

void org_pqrs_driver_VirtualHIDManager_UserClient::stop(IOService* provider) {
  IOLog("org_pqrs_driver_VirtualHIDManager_UserClient::stop\n");

  if (provider_) {
    provider_->detachClient();
  }

  super::stop(provider);
  provider_ = nullptr;
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::clientClose(void) {
  IOLog("org_pqrs_driver_VirtualHIDManager_UserClient::clientClose\n");

  // clear input events.
  pqrs::karabiner_virtualhiddevice::hid_report::pointing_input report;
  pointingInputReportCallback(report);

  if (!terminate()) {
    IOLog("%s Error: terminate failed.\n", __PRETTY_FUNCTION__);
  }

  // DON'T call super::clientClose, which just returns kIOReturnUnsupported.
  return kIOReturnSuccess;
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments,
                                                                      IOExternalMethodDispatch* dispatch, OSObject* target, void* reference) {
  if (selector >= static_cast<uint32_t>(pqrs::karabiner_virtualhiddevice::user_client_method::end_)) {
    return kIOReturnUnsupported;
  }

  dispatch = &(methods_[selector]);
  if (!target) {
    target = this;
  }

  return super::externalMethod(selector, arguments, dispatch, target, reference);
}

#pragma mark - terminate_virtual_hid_keyboard

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::staticTerminateVirtualHIDKeyboardCallback(org_pqrs_driver_VirtualHIDManager_UserClient* target, void* reference, IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  return target->terminateVirtualHIDKeyboardCallback();
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::terminateVirtualHIDKeyboardCallback(void) {
  if (!provider_) {
    return kIOReturnError;
  }

  provider_->terminateVirtualHIDKeyboard();
  return kIOReturnSuccess;
}

#pragma mark - terminate_virtual_hid_pointing

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::staticTerminateVirtualHIDPointingCallback(org_pqrs_driver_VirtualHIDManager_UserClient* target, void* reference, IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  return target->terminateVirtualHIDPointingCallback();
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::terminateVirtualHIDPointingCallback(void) {
  if (!provider_) {
    return kIOReturnError;
  }

  provider_->terminateVirtualHIDPointing();
  return kIOReturnSuccess;
}

#pragma mark - keyboard_input_report

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::staticKeyboardInputReportCallback(org_pqrs_driver_VirtualHIDManager_UserClient* target, void* reference, IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input*>(arguments->structureInput)) {
    return target->keyboardInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::keyboardInputReportCallback(const pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input& input) {
  if (!provider_) {
    return kIOReturnError;
  }

  if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
    auto result = provider_->handleHIDKeyboardReport(report);
    report->release();
    return result;
  }

  return kIOReturnError;
}

#pragma mark - pointing_input_report

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::staticPointingInputReportCallback(org_pqrs_driver_VirtualHIDManager_UserClient* target, void* reference, IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const pqrs::karabiner_virtualhiddevice::hid_report::pointing_input*>(arguments->structureInput)) {
    return target->pointingInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::pointingInputReportCallback(const pqrs::karabiner_virtualhiddevice::hid_report::pointing_input& input) {
  if (!provider_) {
    return kIOReturnError;
  }

  if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
    auto result = provider_->handleHIDPointingReport(report);
    report->release();
    return result;
  }

  return kIOReturnError;
}
