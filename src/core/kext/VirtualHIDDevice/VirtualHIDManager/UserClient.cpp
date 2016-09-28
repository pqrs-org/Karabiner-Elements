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
        // pointing_input_report
        reinterpret_cast<IOExternalMethodAction>(&staticPointingInputReportCallback), // Method pointer.
        0,                                                                            // One scalar input value.
        sizeof(hid_report::pointing_input),                                           // No struct input value.
        0,                                                                            // No scalar output value.
        0                                                                             // No struct output value.
    },
};

bool org_pqrs_driver_VirtualHIDManager_UserClient::initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
  IOLog("%s\n", __PRETTY_FUNCTION__);

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

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::clientClose(void) {
  IOLog("%s\n", __PRETTY_FUNCTION__);

  // clear input events.
  hid_report::pointing_input report;
  pointingInputReportCallback(report);

  return super::clientClose();
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

#pragma mark - pointing_input_report

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::staticPointingInputReportCallback(org_pqrs_driver_VirtualHIDManager_UserClient* target, void* reference, IOExternalMethodArguments* arguments) {
  if (!target) {
    return kIOReturnBadArgument;
  }
  if (!arguments) {
    return kIOReturnBadArgument;
  }

  if (auto input = static_cast<const hid_report::pointing_input*>(arguments->structureInput)) {
    return target->pointingInputReportCallback(*input);
  }

  return kIOReturnBadArgument;
}

IOReturn org_pqrs_driver_VirtualHIDManager_UserClient::pointingInputReportCallback(const hid_report::pointing_input& input) {
  if (!provider_) {
    return kIOReturnError;
  }

  if (auto pointing = provider_->getVirtualHIDPointing()) {
    if (auto report = IOBufferMemoryDescriptor::withBytes(&input, sizeof(input), kIODirectionNone)) {
      auto result = pointing->handleReport(report, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
      report->release();
      return result;
    }
  }

  return kIOReturnError;
}
