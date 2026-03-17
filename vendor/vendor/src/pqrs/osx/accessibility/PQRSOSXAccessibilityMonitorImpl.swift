// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

public typealias PQRSOSXAccessibilityMonitorCallback =
  @Sendable @convention(c) (
    Int32,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    pid_t,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?
  ) -> Void

private struct FrontmostApplication: Sendable, Equatable {
  let name: String?
  let bundleIdentifier: String?
  let bundlePath: String?
  let filePath: String?
  let processIdentifier: pid_t?

  init(processIdentifier: pid_t) {
    let runningApplication = NSRunningApplication(processIdentifier: processIdentifier)
    self.init(runningApplication)
  }

  init(_ runningApplication: NSRunningApplication?) {
    let processIdentifier = runningApplication?.processIdentifier ?? 0

    name = Self.normalize(runningApplication?.localizedName)
    bundleIdentifier = Self.normalize(runningApplication?.bundleIdentifier)
    bundlePath = Self.normalize(runningApplication?.bundleURL?.path)
    filePath = Self.normalize(runningApplication?.executableURL?.path)
    self.processIdentifier = processIdentifier == 0 ? nil : processIdentifier
  }

  static func normalize(_ value: String?) -> String? {
    guard let value, !value.isEmpty else {
      return nil
    }
    return value
  }
}

private struct FocusedUIElement: Sendable, Equatable {
  let role: String?
  let subrole: String?
  let roleDescription: String?
  let title: String?
  let description: String?
  let identifier: String?

  init(_ element: AXUIElement) {
    role = copyAttribute(element, kAXRoleAttribute as CFString)
    subrole = copyAttribute(element, kAXSubroleAttribute as CFString)
    roleDescription = copyAttribute(element, kAXRoleDescriptionAttribute as CFString)
    title = copyAttribute(element, kAXTitleAttribute as CFString)
    description = copyAttribute(element, kAXDescriptionAttribute as CFString)
    identifier = copyAttribute(element, kAXIdentifierAttribute as CFString)
  }
}

private struct Snapshot: Sendable, Equatable {
  let application: FrontmostApplication?
  let focusedUIElement: FocusedUIElement?
}

private func copyAttribute<T>(_ element: AXUIElement, _ attribute: CFString) -> T? {
  var value: CFTypeRef?
  let error = AXUIElementCopyAttributeValue(element, attribute, &value)
  guard error == .success else {
    return nil
  }
  return value as? T
}

private func copyPid(_ element: AXUIElement) -> pid_t? {
  var pid: pid_t = 0
  let error = AXUIElementGetPid(element, &pid)
  guard error == .success, pid != 0 else {
    return nil
  }
  return pid
}

private func copySnapshot() -> Snapshot {
  let systemWideElement = AXUIElementCreateSystemWide()
  let applicationElement: AXUIElement? = copyAttribute(
    systemWideElement, kAXFocusedApplicationAttribute as CFString)
  let focusedUIElement: AXUIElement? = copyAttribute(
    systemWideElement, kAXFocusedUIElementAttribute as CFString)

  let application =
    applicationElement
    .flatMap(copyPid(_:))
    .map(FrontmostApplication.init(processIdentifier:))
    ?? focusedUIElement
    .flatMap(copyPid(_:))
    .map(FrontmostApplication.init(processIdentifier:))

  let workspaceApplication = FrontmostApplication(NSWorkspace.shared.frontmostApplication)

  let focusedElement = focusedUIElement.map(FocusedUIElement.init)

  return Snapshot(
    application: application ?? workspaceApplication,
    focusedUIElement: focusedElement
  )
}

private func withOptionalCString<Result>(
  _ value: String?,
  _ body: (UnsafePointer<CChar>?) -> Result
) -> Result {
  guard let value else {
    return body(nil)
  }

  return value.withCString { pointer in
    body(pointer)
  }
}

private actor PQRSOSXAccessibilityMonitor {
  static let shared = PQRSOSXAccessibilityMonitor()

  private var callback: PQRSOSXAccessibilityMonitorCallback?
  private var monitorTask: Task<Void, Never>?
  private var lastSnapshot = Snapshot(application: nil, focusedUIElement: nil)

  func setCallback(_ callback: @escaping PQRSOSXAccessibilityMonitorCallback) {
    self.callback = callback

    if monitorTask == nil {
      monitorTask = Task {
        await listenLoop()
      }
    }
  }

  func unsetCallback() {
    callback = nil
    monitorTask?.cancel()
    monitorTask = nil
    lastSnapshot = Snapshot(application: nil, focusedUIElement: nil)
  }

  func trigger() async {
    let snapshot = copySnapshot()
    lastSnapshot = snapshot
    await emit(snapshot, force: true)
  }

  private func listenLoop() async {
    while !Task.isCancelled {
      let snapshot = copySnapshot()

      if snapshot != lastSnapshot {
        lastSnapshot = snapshot
        await emit(snapshot, force: false)
      }

      do {
        try await Task.sleep(for: .milliseconds(200))
      } catch {
        break
      }
    }
  }

  private func emit(_ snapshot: Snapshot, force: Bool) async {
    guard let callback else {
      return
    }

    withOptionalCString(snapshot.application?.name) { applicationName in
      withOptionalCString(snapshot.application?.bundleIdentifier) { bundleIdentifier in
        withOptionalCString(snapshot.application?.bundlePath) { bundlePath in
          withOptionalCString(snapshot.application?.filePath) { filePath in
            withOptionalCString(snapshot.focusedUIElement?.role) { role in
              withOptionalCString(snapshot.focusedUIElement?.subrole) { subrole in
                withOptionalCString(snapshot.focusedUIElement?.roleDescription) { roleDescription in
                  withOptionalCString(snapshot.focusedUIElement?.title) { title in
                    withOptionalCString(snapshot.focusedUIElement?.description) { description in
                      withOptionalCString(snapshot.focusedUIElement?.identifier) { identifier in
                        callback(
                          force ? 1 : 0,
                          applicationName,
                          bundleIdentifier,
                          bundlePath,
                          filePath,
                          snapshot.application?.processIdentifier ?? 0,
                          role,
                          subrole,
                          roleDescription,
                          title,
                          description,
                          identifier
                        )
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

@inline(__always)
private func syncCall(_ operation: @Sendable @escaping () async -> Void) {
  let semaphore = DispatchSemaphore(value: 0)
  Task {
    await operation()
    semaphore.signal()
  }
  semaphore.wait()
}

@_cdecl("pqrs_osx_accessibility_monitor_set_callback")
func PQRSOSXAccessibilityMonitorSetCallback(
  _ callback: @escaping PQRSOSXAccessibilityMonitorCallback
) {
  syncCall {
    await PQRSOSXAccessibilityMonitor.shared.setCallback(callback)
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_unset_callback")
func PQRSOSXAccessibilityMonitorUnsetCallback() {
  syncCall {
    await PQRSOSXAccessibilityMonitor.shared.unsetCallback()
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_trigger")
func PQRSOSXAccessibilityMonitorTrigger() {
  syncCall {
    await PQRSOSXAccessibilityMonitor.shared.trigger()
  }
}
