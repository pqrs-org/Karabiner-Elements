// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit

@_cdecl("pqrs_osx_application_set_activation_policy")
func PQRSOSXApplicationSetActivationPolicy(_ policy: pqrs_osx_application_activation_policy_t) {
  let activationPolicy: NSApplication.ActivationPolicy

  switch policy {
  case pqrs_osx_application_activation_policy_regular:
    activationPolicy = .regular
  case pqrs_osx_application_activation_policy_accessory:
    activationPolicy = .accessory
  case pqrs_osx_application_activation_policy_prohibited:
    activationPolicy = .prohibited
  default:
    return
  }

  NSApplication.shared.setActivationPolicy(activationPolicy)
}

@_cdecl("pqrs_osx_application_finish_launching")
func PQRSOSXApplicationFinishLaunching() {
  NSApplication.shared.finishLaunching()
}

@_cdecl("pqrs_osx_application_run")
func PQRSOSXApplicationRun() {
  NSApplication.shared.run()
}

@_cdecl("pqrs_osx_application_stop")
func PQRSOSXApplicationStop() {
  NSApplication.shared.stop(nil)

  // Post a synthetic event to trigger the loop to process and exit.
  if let stopEvent = NSEvent.otherEvent(
    with: .applicationDefined,
    location: .zero,
    modifierFlags: [],
    timestamp: 0,
    windowNumber: 0,
    context: nil,
    subtype: 0,
    data1: 0,
    data2: 0
  ) {
    NSApplication.shared.postEvent(stopEvent, atStart: false)
  }
}
