// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit

private class PQRSOSXFrontmostApplicationMonitor {
  static let shared = PQRSOSXFrontmostApplicationMonitor()

  struct Callback {
    let function: pqrs_osx_frontmost_application_monitor_callback
    let context: UnsafeMutableRawPointer?
  }

  // Use DispatchQueue instead of Swift Concurrency for macOS 11 support with C++ project.
  // There is Swift Concurrency implementation in `appendix/PQRSOSXFrontmostApplicationMonitorImpl.swift`.
  let queue = DispatchQueue(label: "org.pqrs.osx.frontmost_application_monitor")
  var callbacks: [Callback] = []

  init() {
    let sharedWorkspace = NSWorkspace.shared
    let notificationCenter = sharedWorkspace.notificationCenter

    notificationCenter.addObserver(
      forName: NSWorkspace.didActivateApplicationNotification,
      object: sharedWorkspace,
      queue: nil
    ) { [weak self] note in
      guard let self = self else { return }

      guard let userInfo = note.userInfo else {
        print("Missing notification info on NSWorkspace.didActivateApplicationNotification")
        return
      }

      guard
        let runningApplication = userInfo[NSWorkspace.applicationUserInfoKey]
          as? NSRunningApplication
      else {
        print("Missing runningApplication on NSWorkspace.didActivateApplicationNotification")
        return
      }

      self.queue.async { [weak self] in
        guard let self = self else { return }

        self.runCallback(runningApplication)
      }
    }
  }

  func register(_ callback: Callback) {
    for c in callbacks {
      let ptr1 = unsafeBitCast(c.function, to: Optional<UnsafeRawPointer>.self)
      let ptr2 = unsafeBitCast(callback.function, to: Optional<UnsafeRawPointer>.self)
      if ptr1 == ptr2 && c.context == callback.context {
        return
      }
    }

    callbacks.append(callback)
  }

  func unregister(_ callback: Callback) {
    callbacks.removeAll(where: {
      let ptr1 = unsafeBitCast($0.function, to: Optional<UnsafeRawPointer>.self)
      let ptr2 = unsafeBitCast(callback.function, to: Optional<UnsafeRawPointer>.self)
      return ptr1 == ptr2 && $0.context == callback.context
    })
  }

  func runCallback(_ runningApplication: NSRunningApplication) {
    for c in callbacks {
      let bundleIdentifier = runningApplication.bundleIdentifier ?? ""
      let path = runningApplication.executableURL?.path ?? ""

      bundleIdentifier.utf8CString.withUnsafeBufferPointer { bundleIdentifierPtr in
        path.utf8CString.withUnsafeBufferPointer { pathPtr in
          c.function(
            bundleIdentifierPtr.baseAddress,
            pathPtr.baseAddress,
            c.context
          )
        }
      }
    }
  }

  func runCallbackWithFrontmostApplication() {
    if let runningApplication = NSWorkspace.shared.frontmostApplication {
      runCallback(runningApplication)
    }
  }
}

@_cdecl("pqrs_osx_frontmost_application_monitor_register")
func register(
  _ function: pqrs_osx_frontmost_application_monitor_callback,
  _ context: pqrs_osx_frontmost_application_monitor_callback_context
) {
  PQRSOSXFrontmostApplicationMonitor.shared.queue.async {
    PQRSOSXFrontmostApplicationMonitor.shared.register(
      PQRSOSXFrontmostApplicationMonitor.Callback(
        function: function,
        context: context
      ))

    PQRSOSXFrontmostApplicationMonitor.shared.runCallbackWithFrontmostApplication()
  }
}

@_cdecl("pqrs_osx_frontmost_application_monitor_unregister")
func unregister(
  _ function: pqrs_osx_frontmost_application_monitor_callback,
  _ context: pqrs_osx_frontmost_application_monitor_callback_context
) {
  PQRSOSXFrontmostApplicationMonitor.shared.queue.async {
    PQRSOSXFrontmostApplicationMonitor.shared.unregister(
      PQRSOSXFrontmostApplicationMonitor.Callback(
        function: function,
        context: context
      ))
  }
}
