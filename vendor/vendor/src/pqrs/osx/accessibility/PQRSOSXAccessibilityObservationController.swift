// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

private let observedAccessibilityNotifications: [CFString] = [
  kAXFocusedUIElementChangedNotification as CFString,
  kAXFocusedWindowChangedNotification as CFString,
  kAXMainWindowChangedNotification as CFString,
]

private let accessibilityObserverCallback: AXObserverCallback = { _, _, notification, _ in
  Task {
    if observedAccessibilityNotifications.contains(notification) {
      await PQRSOSXAccessibilityMonitor.shared.requestRefresh(force: false)
    }
  }
}

@MainActor
final class PQRSOSXAccessibilityObservationController {
  private var activationObserver: NSObjectProtocol?
  private var terminationObserver: NSObjectProtocol?
  // PIDs that have been observed through NSWorkspace activation notifications.
  private var workspaceKnownPIDs: Set<pid_t> = []
  // PIDs discovered outside NSWorkspace that still need AXObserver-based tracking.
  private var observerManagedPIDs: Set<pid_t> = []
  private var observersByPID: [pid_t: AXObserver] = [:]
  // The current frontmost PID used to keep frontmost-app observation attached.
  private var frontmostProcessIdentifier: pid_t?

  func start() {
    guard activationObserver == nil, terminationObserver == nil else {
      return
    }

    activationObserver = NSWorkspace.shared.notificationCenter.addObserver(
      forName: NSWorkspace.didActivateApplicationNotification,
      object: nil,
      queue: nil
    ) { [weak self] _ in
      guard let self else {
        return
      }

      Task { @MainActor in
        self.requestRefresh()
      }
    }

    terminationObserver = NSWorkspace.shared.notificationCenter.addObserver(
      forName: NSWorkspace.didTerminateApplicationNotification,
      object: nil,
      queue: nil
    ) { [weak self] notification in
      guard let self else {
        return
      }

      Task { @MainActor in
        let processIdentifier =
          (notification.userInfo?[NSWorkspace.applicationUserInfoKey] as? NSRunningApplication)?
          .processIdentifier

        self.pruneProcessIdentifier(processIdentifier)
        self.requestRefresh()
      }
    }

    requestRefresh()
  }

  func stop() {
    if let activationObserver {
      NSWorkspace.shared.notificationCenter.removeObserver(activationObserver)
      self.activationObserver = nil
    }

    if let terminationObserver {
      NSWorkspace.shared.notificationCenter.removeObserver(terminationObserver)
      self.terminationObserver = nil
    }

    for processIdentifier in Array(observersByPID.keys) {
      detachObserver(processIdentifier: processIdentifier)
    }

    workspaceKnownPIDs.removeAll()
    observerManagedPIDs.removeAll()
    observersByPID.removeAll()
    frontmostProcessIdentifier = nil
  }

  func registerProcessIdentifier(_ processIdentifier: pid_t?, detectionSource: DetectionSource) {
    guard let processIdentifier, processIdentifier != 0 else {
      return
    }

    switch detectionSource {
    case .workspace:
      workspaceKnownPIDs.insert(processIdentifier)
      observerManagedPIDs.remove(processIdentifier)

    case .axObserver:
      guard !workspaceKnownPIDs.contains(processIdentifier) else {
        return
      }

      observerManagedPIDs.insert(processIdentifier)

    case .none:
      break
    }
  }

  func pruneProcessIdentifier(_ processIdentifier: pid_t?) {
    guard let processIdentifier, processIdentifier != 0 else {
      return
    }

    workspaceKnownPIDs.remove(processIdentifier)
    observerManagedPIDs.remove(processIdentifier)

    if frontmostProcessIdentifier == processIdentifier {
      frontmostProcessIdentifier = nil
    }

    detachObserver(processIdentifier: processIdentifier)
  }

  func pruneStaleProcessIdentifiers() {
    let knownProcessIdentifiers =
      workspaceKnownPIDs
      .union(observerManagedPIDs)
      .union(observersByPID.keys)

    for processIdentifier in knownProcessIdentifiers {
      if NSRunningApplication(processIdentifier: processIdentifier) == nil {
        pruneProcessIdentifier(processIdentifier)
      }
    }
  }

  private func requestRefresh() {
    Task {
      await PQRSOSXAccessibilityMonitor.shared.requestRefresh(force: false)
    }
  }

  func syncObservers(frontmostProcessIdentifier: pid_t?) {
    self.frontmostProcessIdentifier = frontmostProcessIdentifier

    var targetPIDs = observerManagedPIDs.subtracting(workspaceKnownPIDs)

    if let frontmostProcessIdentifier, frontmostProcessIdentifier != 0 {
      targetPIDs.insert(frontmostProcessIdentifier)
    }

    let stalePIDs = Set(observersByPID.keys).subtracting(targetPIDs)
    for processIdentifier in stalePIDs {
      detachObserver(processIdentifier: processIdentifier)
    }

    for processIdentifier in targetPIDs {
      attachObserver(processIdentifier: processIdentifier)
    }
  }

  private func attachObserver(processIdentifier: pid_t) {
    guard processIdentifier != 0 else {
      return
    }

    guard observersByPID[processIdentifier] == nil else {
      return
    }

    var observer: AXObserver?
    let error = AXObserverCreate(processIdentifier, accessibilityObserverCallback, &observer)
    guard error == .success, let observer else {
      return
    }

    let applicationElement = AXUIElementCreateApplication(processIdentifier)
    var registered = false

    for notification in observedAccessibilityNotifications {
      let error = AXObserverAddNotification(observer, applicationElement, notification, nil)
      if error == .success {
        registered = true
      }
    }

    guard registered else {
      return
    }

    CFRunLoopAddSource(
      CFRunLoopGetMain(),
      AXObserverGetRunLoopSource(observer),
      .commonModes
    )

    observersByPID[processIdentifier] = observer
  }

  private func detachObserver(processIdentifier: pid_t) {
    if let axObserver = observersByPID.removeValue(forKey: processIdentifier) {
      CFRunLoopRemoveSource(
        CFRunLoopGetMain(),
        AXObserverGetRunLoopSource(axObserver),
        .commonModes
      )
    }
  }
}
