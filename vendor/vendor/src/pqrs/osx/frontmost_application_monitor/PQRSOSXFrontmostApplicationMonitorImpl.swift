// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import os

// We can add @Sendable since the `frontmost_application_monitor::static_cpp_callback` is sendable.
public typealias PQRSOSXFrontmostApplicationMonitorCallback =
  @Sendable @convention(c) (
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    pid_t
  ) -> Void

private struct FrontmostApplication: Sendable {
  let bundleIdentifier: String
  let bundlePath: String
  let filePath: String
  let processIdentifier: pid_t

  init(_ runningApplication: NSRunningApplication) {
    bundleIdentifier = runningApplication.bundleIdentifier ?? ""
    bundlePath = runningApplication.bundleURL?.path ?? ""
    filePath = runningApplication.executableURL?.path ?? ""
    processIdentifier = runningApplication.processIdentifier
  }
}

private actor PQRSOSXFrontmostApplicationMonitor {
  static let shared = PQRSOSXFrontmostApplicationMonitor()

  private var callback: PQRSOSXFrontmostApplicationMonitorCallback?
  private var notificationsTask: Task<Void, Never>?

  func setCallback(_ callback: PQRSOSXFrontmostApplicationMonitorCallback) {
    self.callback = callback

    if notificationsTask == nil {
      notificationsTask = Task {
        await listenLoop()
      }
    }
  }

  func unsetCallback() {
    callback = nil

    notificationsTask?.cancel()
    notificationsTask = nil
  }

  private func listenLoop() async {
    let stream = NSWorkspace.shared.notificationCenter.notifications(
      named: NSWorkspace.didActivateApplicationNotification
    )
    .compactMap { note -> FrontmostApplication? in
      guard
        let userInfo = note.userInfo,
        let runningApplication = userInfo[NSWorkspace.applicationUserInfoKey]
          as? NSRunningApplication
      else { return nil }

      return FrontmostApplication(runningApplication)
    }

    for await frontmostApplication in stream {
      runCallback(frontmostApplication)
    }
  }

  func runCallback(_ frontmostApplication: FrontmostApplication) {
    guard let callback = callback else { return }

    frontmostApplication.bundleIdentifier.withCString { bundleIdentifierPtr in
      frontmostApplication.bundlePath.withCString { bundlePathPtr in
        frontmostApplication.filePath.withCString { filePathPtr in
          callback(
            bundleIdentifierPtr,
            bundlePathPtr,
            filePathPtr,
            frontmostApplication.processIdentifier
          )
        }
      }
    }
  }

  func runCallbackWithFrontmostApplication() {
    if let runningApplication = NSWorkspace.shared.frontmostApplication {
      runCallback(FrontmostApplication(runningApplication))
    }
  }
}

// If SetCallback and Trigger are called back-to-back,
// just using regular Tasks doesn't guarantee execution order within the actor.
// There's a chance Trigger might run before SetCallback.
// To enforce the correct call order,methods invoked from C should
// wait for execution to complete before returning to the caller.
@inline(__always)
private func syncCall(_ operation: @Sendable @escaping () async -> Void) {
  let sem = DispatchSemaphore(value: 0)
  Task {
    await operation()
    sem.signal()
  }
  sem.wait()
}

@_cdecl("pqrs_osx_frontmost_application_monitor_set_callback")
func PQRSOSXFrontmostApplicationMonitorSetCallback(
  _ callback: PQRSOSXFrontmostApplicationMonitorCallback
) {
  syncCall {
    await PQRSOSXFrontmostApplicationMonitor.shared.setCallback(callback)
  }
}

@_cdecl("pqrs_osx_frontmost_application_monitor_unset_callback")
func PQRSOSXFrontmostApplicationMonitorUnsetCallback() {
  syncCall {
    await PQRSOSXFrontmostApplicationMonitor.shared.unsetCallback()
  }
}

@_cdecl("pqrs_osx_frontmost_application_monitor_trigger")
func PQRSOSXFrontmostApplicationMonitorTrigger() {
  syncCall {
    await PQRSOSXFrontmostApplicationMonitor.shared.runCallbackWithFrontmostApplication()
  }
}
