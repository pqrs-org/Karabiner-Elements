// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

private let accessibilityObserverCallback: AXObserverCallback = { _, element, notification, _ in
  Task {
    await PQRSOSXAccessibilityMonitor.shared.handleAccessibilityNotification(
      notification,
      processIdentifier: copyPid(element)
    )
  }
}

@MainActor
final class PQRSOSXAccessibilityObservationController {
  private let observedNotifications: [CFString] = [
    kAXFocusedUIElementChangedNotification as CFString,
    kAXFocusedWindowChangedNotification as CFString,
    kAXMainWindowChangedNotification as CFString,
  ]

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

    if let processIdentifier = NSWorkspace.shared.frontmostApplication?.processIdentifier,
      processIdentifier != 0
    {
      workspaceKnownPIDs.insert(processIdentifier)
    }

    activationObserver = NSWorkspace.shared.notificationCenter.addObserver(
      forName: NSWorkspace.didActivateApplicationNotification,
      object: nil,
      queue: nil
    ) { [weak self] notification in
      guard let self else {
        return
      }

      Task { @MainActor in
        self.handleDidActivateApplication(notification)
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
        self.handleDidTerminateApplication(notification)
        self.requestRefresh()
      }
    }

    syncObservers(
      frontmostProcessIdentifier: NSWorkspace.shared.frontmostApplication?
        .processIdentifier)
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

  func registerObserverManagedProcessIdentifier(_ processIdentifier: pid_t?) {
    guard let processIdentifier, processIdentifier != 0 else {
      return
    }

    guard !workspaceKnownPIDs.contains(processIdentifier) else {
      return
    }

    observerManagedPIDs.insert(processIdentifier)
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

  func handleAccessibilityNotification(_ notification: CFString, processIdentifier: pid_t?) {
    if observedNotifications.contains(notification) {
      if let processIdentifier, processIdentifier != 0,
        !workspaceKnownPIDs.contains(processIdentifier)
      {
        observerManagedPIDs.insert(processIdentifier)
      }

      requestRefresh()
    }
  }

  private func requestRefresh() {
    Task {
      await PQRSOSXAccessibilityMonitor.shared.requestRefresh(force: false)
    }
  }

  private func handleDidActivateApplication(_ notification: Notification) {
    let processIdentifier =
      (notification.userInfo?[NSWorkspace.applicationUserInfoKey] as? NSRunningApplication)?
      .processIdentifier
      ?? NSWorkspace.shared.frontmostApplication?.processIdentifier

    guard let processIdentifier, processIdentifier != 0 else {
      syncObservers(
        frontmostProcessIdentifier: NSWorkspace.shared.frontmostApplication?
          .processIdentifier)
      requestRefresh()
      return
    }

    workspaceKnownPIDs.insert(processIdentifier)
    observerManagedPIDs.remove(processIdentifier)
    syncObservers(frontmostProcessIdentifier: processIdentifier)
    requestRefresh()
  }

  private func handleDidTerminateApplication(_ notification: Notification) {
    let processIdentifier =
      (notification.userInfo?[NSWorkspace.applicationUserInfoKey] as? NSRunningApplication)?
      .processIdentifier

    pruneProcessIdentifier(processIdentifier)
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

    for notification in observedNotifications {
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
