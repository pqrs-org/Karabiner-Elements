#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
END_IOKIT_INCLUDE;

#include "UserClient.hpp"

#define super IOUserClient

OSDefineMetaClassAndStructors(org_pqrs_driver_VirtualHIDManager_UserClient, IOUserClient);

IOExternalMethodDispatch org_pqrs_driver_VirtualHIDManager_UserClient::methods_[static_cast<size_t>(virtual_hid_manager_user_client_method::end_)] = {
    {
        // keyboard_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticKeyboardInputReportCallback), // Method pointer.
        0,                                                                            // One scalar input value.
        sizeof(hid_report::keyboard_input),                                           // No struct input value.
        0,                                                                            // No scalar output value.
        0                                                                             // No struct output value.
    },
};

bool org_pqrs_driver_VirtualHIDManager_UserClient::initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
  if (clientHasPrivilege(owningTask, kIOClientPrivilegeAdministrator) != KERN_SUCCESS) {
    IOLog("%s Error: clientHasPrivilege failed.\n", __PRETTY_FUNCTION__);
    return false;
  }

  if (!super::initWithTask(owningTask, securityToken, type)) {
    return false;
  }

  owner_ = owningTask;
  provider_ = nullptr;

  return true;
}

bool org_pqrs_driver_VirtualHIDManager_UserClient::start(IOService* provider) {
  provider_ = OSDynamicCast(org_pqrs_driver_VirtualHIDManager, provider);
  if (!provider_) {
    IOLog("%s Error: provider_ == nullptr\n", __PRETTY_FUNCTION__);
    return false;
  }

  if (!super::start(provider)) {
    return false;
  }

  return true;
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments,
                                                                      IOExternalMethodDispatch* dispatch, OSObject* target, void* reference) {
  if (selector >= static_cast<uint32_t>(virtual_hid_manager_user_client_method::end_)) {
    return kIOReturnUnsupported;
  }

  dispatch = &(methods_[selector]);
  if (!target) {
    target = this;
  }

  return super::externalMethod(selector, arguments, dispatch, target, reference);
}

#pragma mark - report

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::staticKeyboardInputReportCallback(org_pqrs_driver_VirtualHIDManager_UserClient* target, void* reference, IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  auto input = static_cast<const hid_report::keyboard_input*>(arguments->structureInput);
  if (!input) {
    return kIOReturnBadArgument;
  }

  return target->keyboardInputReportCallback(*input);
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::keyboardInputReportCallback(const hid_report::keyboard_input& input) {
  if (!provider_) {
    return kIOReturnError;
  }

  auto keyboard = provider_->getVirtualHIDKeyboard();
  if (!keyboard) {
    return kIOReturnError;
  }

  IOMemoryDescriptor* report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone);
  if (!report) {
    return kIOReturnError;
  }

  IOReturn result = keyboard->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
  report->release();
  return result;
}
