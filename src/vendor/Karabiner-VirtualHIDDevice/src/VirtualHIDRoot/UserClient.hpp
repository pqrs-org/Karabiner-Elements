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
#include "VirtualHIDEventService.hpp"
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
#include "KernelVersion.hpp"
#include "ServiceDetector.hpp"

  // ----------------------------------------
  // VirtualHIDKeyboard
  static IOReturn staticInitializeVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                             void* reference,
                                                             IOExternalMethodArguments* arguments);
  IOReturn initializeVirtualHIDKeyboardCallback(void);

  static IOReturn staticTerminateVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                            void* reference,
                                                            IOExternalMethodArguments* arguments);
  IOReturn terminateVirtualHIDKeyboardCallback(void);

  static IOReturn staticDispatchKeyboardEventCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                      void* reference,
                                                      IOExternalMethodArguments* arguments);
  IOReturn dispatchKeyboardEventCallback(const pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event& keyboard_event);

  static IOReturn staticPostKeyboardInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments);
  IOReturn postKeyboardInputReportCallback(const pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input& input);

  static IOReturn staticResetVirtualHIDKeyboardCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments);
  IOReturn resetVirtualHIDKeyboardCallback(void);

  // ----------------------------------------
  // VirtualHIDPointing
  static IOReturn staticInitializeVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                             void* reference,
                                                             IOExternalMethodArguments* arguments);
  IOReturn initializeVirtualHIDPointingCallback(void);

  static IOReturn staticTerminateVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                            void* reference,
                                                            IOExternalMethodArguments* arguments);
  IOReturn terminateVirtualHIDPointingCallback(void);

  static IOReturn staticPostPointingInputReportCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments);
  IOReturn postPointingInputReportCallback(const pqrs::karabiner_virtual_hid_device::hid_report::pointing_input& input);

  static IOReturn staticResetVirtualHIDPointingCallback(VIRTUAL_HID_ROOT_USERCLIENT_CLASS* target,
                                                        void* reference,
                                                        IOExternalMethodArguments* arguments);
  IOReturn resetVirtualHIDPointingCallback(void);

  // ----------------------------------------
  static bool isTargetHIDInterface(IOService* newService, IOService* refCon);

  bool initializeVirtualHIDEventService(IOHIDInterface* hidInterface);
  void terminateVirtualHIDEventService(void);

  // ----------------------------------------
  ServiceDetector hidInterfaceDetector_;
  static IOExternalMethodDispatch methods_[static_cast<size_t>(pqrs::karabiner_virtual_hid_device::user_client_method::end_)];
  int kernelMajorReleaseVersion_;
  VIRTUAL_HID_KEYBOARD_CLASS* virtualHIDKeyboard_;
  VIRTUAL_HID_POINTING_CLASS* virtualHIDPointing_;
  VIRTUAL_HID_EVENT_SERVICE_CLASS* virtualHIDEventService_;
};
