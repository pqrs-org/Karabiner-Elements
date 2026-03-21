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

struct FrontmostApplication: Sendable, Equatable {
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

struct FocusedUIElement: Sendable, Equatable {
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

struct Snapshot: Sendable, Equatable {
  let application: FrontmostApplication?
  let focusedUIElement: FocusedUIElement?
}

func copyAttribute<T>(_ element: AXUIElement, _ attribute: CFString) -> T? {
  var value: CFTypeRef?
  let error = AXUIElementCopyAttributeValue(element, attribute, &value)
  guard error == .success else {
    return nil
  }
  return value as? T
}

func copyPid(_ element: AXUIElement) -> pid_t? {
  var pid: pid_t = 0
  let error = AXUIElementGetPid(element, &pid)
  guard error == .success, pid != 0 else {
    return nil
  }
  return pid
}

@MainActor
func copySnapshot(cachedApplication: FrontmostApplication?) -> Snapshot {
  let systemWideElement = AXUIElementCreateSystemWide()
  let applicationElement: AXUIElement? = copyAttribute(
    systemWideElement, kAXFocusedApplicationAttribute as CFString)
  let systemWideFocusedUIElement: AXUIElement? = copyAttribute(
    systemWideElement, kAXFocusedUIElementAttribute as CFString)

  let processIdentifier =
    applicationElement
    .flatMap(copyPid(_:))
    ?? systemWideFocusedUIElement
    .flatMap(copyPid(_:))

  let resolvedApplication: FrontmostApplication? =
    if let processIdentifier {
      if cachedApplication?.processIdentifier == processIdentifier {
        cachedApplication
      } else {
        FrontmostApplication(processIdentifier: processIdentifier)
      }
    } else {
      FrontmostApplication(NSWorkspace.shared.frontmostApplication)
    }

  let applicationFocusedUIElement: AXUIElement? =
    resolvedApplication?.processIdentifier
    .map(AXUIElementCreateApplication)
    .flatMap {
      copyAttribute($0, kAXFocusedUIElementAttribute as CFString) as AXUIElement?
    }

  let focusedElement = (applicationFocusedUIElement ?? systemWideFocusedUIElement).map(
    FocusedUIElement.init)

  return Snapshot(
    application: resolvedApplication,
    focusedUIElement: focusedElement
  )
}

func withOptionalCString<Result>(
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

// Some transient system UIs such as Spotlight do not reliably surface through
// app-activation or focused-element notifications, so keep a modest fallback poll.
let fallbackPollingInterval = Duration.milliseconds(500)
let staleProcessCleanupInterval = Duration.seconds(10)
