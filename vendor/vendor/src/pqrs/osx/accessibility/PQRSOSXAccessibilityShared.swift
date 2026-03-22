// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

public typealias PQRSOSXAccessibilityMonitorCallback =
  @Sendable @convention(c) (
    Int32,
    UnsafePointer<pqrs_osx_accessibility_snapshot>?
  ) -> Void

struct WindowPosition: Sendable, Equatable {
  let x: Double
  let y: Double
}

struct WindowSize: Sendable, Equatable {
  let width: Double
  let height: Double
}

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
  let windowPosition: WindowPosition?
  let windowSize: WindowSize?

  init(_ element: AXUIElement, applicationElement: AXUIElement?) {
    role = copyAttribute(element, kAXRoleAttribute as CFString)
    subrole = copyAttribute(element, kAXSubroleAttribute as CFString)
    roleDescription = copyAttribute(element, kAXRoleDescriptionAttribute as CFString)
    title = copyAttribute(element, kAXTitleAttribute as CFString)
    description = copyAttribute(element, kAXDescriptionAttribute as CFString)
    identifier = copyAttribute(element, kAXIdentifierAttribute as CFString)
    let windowElement =
      (copyAttribute(element, kAXWindowAttribute as CFString) as AXUIElement?)
      ?? applicationElement.flatMap {
        copyAttribute($0, kAXFocusedWindowAttribute as CFString) as AXUIElement?
      }

    windowPosition = windowElement.flatMap {
      copyCGPointAttribute($0, kAXPositionAttribute as CFString)
    }.map {
      WindowPosition(x: Double($0.x), y: Double($0.y))
    }
    windowSize = windowElement.flatMap {
      copyCGSizeAttribute($0, kAXSizeAttribute as CFString)
    }.map {
      WindowSize(width: Double($0.width), height: Double($0.height))
    }
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

func copyCGPointAttribute(_ element: AXUIElement, _ attribute: CFString) -> CGPoint? {
  guard let value: AXValue = copyAttribute(element, attribute) else {
    return nil
  }

  guard AXValueGetType(value) == .cgPoint else {
    return nil
  }

  var point = CGPoint.zero
  guard AXValueGetValue(value, .cgPoint, &point) else {
    return nil
  }

  return point
}

func copyCGSizeAttribute(_ element: AXUIElement, _ attribute: CFString) -> CGSize? {
  guard let value: AXValue = copyAttribute(element, attribute) else {
    return nil
  }

  guard AXValueGetType(value) == .cgSize else {
    return nil
  }

  var size = CGSize.zero
  guard AXValueGetValue(value, .cgSize, &size) else {
    return nil
  }

  return size
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

  let focusedElement = (applicationFocusedUIElement ?? systemWideFocusedUIElement).map {
    FocusedUIElement($0, applicationElement: applicationElement)
  }

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
