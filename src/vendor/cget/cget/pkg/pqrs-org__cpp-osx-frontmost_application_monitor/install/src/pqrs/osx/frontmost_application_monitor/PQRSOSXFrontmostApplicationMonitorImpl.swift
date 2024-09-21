// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import os

private class PQRSOSXFrontmostApplicationMonitor {
  static let shared = PQRSOSXFrontmostApplicationMonitor()

  var callback: pqrs_osx_frontmost_application_monitor_callback?
  let lock = OSAllocatedUnfairLock()

  init() {
    let sharedWorkspace = NSWorkspace.shared
    let notificationCenter = sharedWorkspace.notificationCenter

    notificationCenter.addObserver(
      forName: NSWorkspace.didActivateApplicationNotification,
      object: sharedWorkspace,
      queue: nil
    ) { note in
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

      let bundleIdentifier = runningApplication.bundleIdentifier ?? ""
      let path = runningApplication.executableURL?.path ?? ""

      self.runCallback(bundleIdentifier: bundleIdentifier, path: path)
    }
  }

  func setCallback(_ callback: pqrs_osx_frontmost_application_monitor_callback) {
    lock.withLock {
      self.callback = callback
    }
  }

  func unsetCallback() {
    lock.withLock {
      callback = nil
    }
  }

  func runCallback(bundleIdentifier: String, path: String) {
    lock.withLock {
      bundleIdentifier.utf8CString.withUnsafeBufferPointer { bundleIdentifierPtr in
        path.utf8CString.withUnsafeBufferPointer { pathPtr in
          callback?(
            bundleIdentifierPtr.baseAddress,
            pathPtr.baseAddress
          )
        }
      }
    }
  }

  func runCallbackWithFrontmostApplication() {
    if let runningApplication = NSWorkspace.shared.frontmostApplication {
      let bundleIdentifier = runningApplication.bundleIdentifier ?? ""
      let path = runningApplication.executableURL?.path ?? ""

      runCallback(bundleIdentifier: bundleIdentifier, path: path)
    }
  }
}

@_cdecl("pqrs_osx_frontmost_application_monitor_set_callback")
func PQRSOSXFrontmostApplicationMonitorSetCallback(
  _ callback: pqrs_osx_frontmost_application_monitor_callback
) {
  PQRSOSXFrontmostApplicationMonitor.shared.setCallback(callback)
}

@_cdecl("pqrs_osx_frontmost_application_monitor_unset_callback")
func PQRSOSXFrontmostApplicationMonitorUnsetCallback() {
  PQRSOSXFrontmostApplicationMonitor.shared.unsetCallback()
}

@_cdecl("pqrs_osx_frontmost_application_monitor_trigger")
func PQRSOSXFrontmostApplicationMonitorTrigger() {
  PQRSOSXFrontmostApplicationMonitor.shared.runCallbackWithFrontmostApplication()
}
