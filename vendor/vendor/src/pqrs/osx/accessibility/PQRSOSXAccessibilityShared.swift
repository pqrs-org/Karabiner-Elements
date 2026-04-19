// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

enum DetectionSource: Int32, Sendable, Equatable {
  case none = 0
  case workspace = 1
  case axObserver = 2
}

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

struct WindowGeometry: Sendable, Equatable {
  let position: WindowPosition?
  let size: WindowSize?

  var isEmpty: Bool {
    position == nil && size == nil
  }
}

enum WindowGeometrySource: Sendable, Equatable {
  case none
  case ax
  case coreGraphics
}

struct FrontmostWindowGeometryCacheEntry {
  let geometry: WindowGeometry?
  let timestamp: TimeInterval
}

@MainActor
var frontmostWindowGeometryCache: [pid_t: FrontmostWindowGeometryCacheEntry] = [:]

struct FrontmostApplication: Sendable, Equatable {
  let name: String?
  let bundleIdentifier: String?
  let bundlePath: String?
  let filePath: String?
  let processIdentifier: pid_t?
  let detectionSource: DetectionSource

  init(processIdentifier: pid_t, detectionSource: DetectionSource = .none) {
    let runningApplication = NSRunningApplication(processIdentifier: processIdentifier)
    self.init(runningApplication, detectionSource: detectionSource)
  }

  init(_ runningApplication: NSRunningApplication?, detectionSource: DetectionSource = .none) {
    let processIdentifier = runningApplication?.processIdentifier ?? 0

    name = Self.normalize(runningApplication?.localizedName)
    bundleIdentifier = Self.normalize(runningApplication?.bundleIdentifier)
    bundlePath = Self.normalize(runningApplication?.bundleURL?.path)
    filePath = Self.normalize(runningApplication?.executableURL?.path)
    self.processIdentifier = processIdentifier == 0 ? nil : processIdentifier
    self.detectionSource = detectionSource
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
  let windowGeometrySource: WindowGeometrySource

  init(windowGeometry: WindowGeometry?) {
    role = nil
    subrole = nil
    roleDescription = nil
    title = nil
    description = nil
    identifier = nil
    windowPosition = windowGeometry?.position
    windowSize = windowGeometry?.size
    windowGeometrySource = windowGeometry?.isEmpty == false ? .coreGraphics : .none
  }

  @MainActor
  init(
    _ element: AXUIElement,
    applicationElement: AXUIElement?,
    processIdentifier: pid_t?
  ) {
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

    let axWindowGeometry =
      windowElement
      .map(copyWindowGeometry(_:))
      .flatMap { $0.isEmpty ? nil : $0 }

    let fallbackWindowGeometry: WindowGeometry? =
      if axWindowGeometry == nil, let processIdentifier {
        copyFrontmostWindowGeometry(processIdentifier)
      } else {
        nil
      }

    let windowGeometry =
      axWindowGeometry
      ?? fallbackWindowGeometry

    windowPosition = windowGeometry?.position
    windowSize = windowGeometry?.size
    if axWindowGeometry != nil {
      windowGeometrySource = .ax
    } else if fallbackWindowGeometry != nil {
      windowGeometrySource = .coreGraphics
    } else {
      windowGeometrySource = .none
    }
  }
}

struct Snapshot: Sendable, Equatable {
  let application: FrontmostApplication?
  let focusedUIElement: FocusedUIElement?
}

@MainActor
func copyFrontmostProcessIdentifier() -> pid_t? {
  let systemWideElement = AXUIElementCreateSystemWide()
  let (_, applicationElement) = copyAXUIElementAttributeValue(
    systemWideElement,
    kAXFocusedApplicationAttribute as CFString
  )
  let (_, systemWideFocusedUIElement) = copyAXUIElementAttributeValue(
    systemWideElement,
    kAXFocusedUIElementAttribute as CFString
  )

  return applicationElement
    .flatMap(copyPid(_:))
    ?? systemWideFocusedUIElement
    .flatMap(copyPid(_:))
    ?? NSWorkspace.shared.frontmostApplication?.processIdentifier
}

@MainActor
func resolveFrontmostApplication(
  cachedApplication: FrontmostApplication?,
  workspaceFrontmostApplication: NSRunningApplication?,
  applicationElement: AXUIElement?,
  systemWideFocusedUIElement: AXUIElement?,
  handleProcessIdentifier: (pid_t?, DetectionSource) -> Void
) -> FrontmostApplication? {
  let axProcessIdentifier =
    applicationElement
    .flatMap(copyPid(_:))
    ?? systemWideFocusedUIElement
    .flatMap(copyPid(_:))

  let workspaceProcessIdentifier = workspaceFrontmostApplication?.processIdentifier
  let processIdentifier: pid_t? =
    if let axProcessIdentifier, axProcessIdentifier != 0 {
      axProcessIdentifier
    } else if let workspaceProcessIdentifier, workspaceProcessIdentifier != 0 {
      workspaceProcessIdentifier
    } else {
      nil
    }

  let detectionSource: DetectionSource
  if let axProcessIdentifier, axProcessIdentifier != 0 {
    if workspaceProcessIdentifier == nil
      || workspaceProcessIdentifier == 0
      || workspaceProcessIdentifier != axProcessIdentifier
    {
      handleProcessIdentifier(workspaceProcessIdentifier, .workspace)
      handleProcessIdentifier(axProcessIdentifier, .axObserver)
      detectionSource = .axObserver
    } else {
      handleProcessIdentifier(axProcessIdentifier, .workspace)
      detectionSource = .workspace
    }
  } else if let workspaceProcessIdentifier, workspaceProcessIdentifier != 0 {
    handleProcessIdentifier(workspaceProcessIdentifier, .workspace)
    detectionSource = .workspace
  } else {
    detectionSource = .none
  }

  return if let processIdentifier {
    if cachedApplication?.processIdentifier == processIdentifier,
      cachedApplication?.detectionSource == detectionSource
    {
      cachedApplication
    } else {
      FrontmostApplication(
        processIdentifier: processIdentifier,
        detectionSource: detectionSource
      )
    }
  } else {
    FrontmostApplication(
      workspaceFrontmostApplication,
      detectionSource: detectionSource
    )
  }
}

func copyAttribute<T>(_ element: AXUIElement, _ attribute: CFString) -> T? {
  let (error, value) = copyAttributeValue(element, attribute)
  guard error == .success, let value else {
    return nil
  }

  return value as? T
}

func copyAXUIElementAttributeValue(_ element: AXUIElement, _ attribute: CFString) -> (
  AXError, AXUIElement?
) {
  let (error, value) = copyAttributeValue(element, attribute)
  guard error == .success, let value else {
    return (error, nil)
  }

  guard CFGetTypeID(value) == AXUIElementGetTypeID() else {
    return (error, nil)
  }

  return (error, unsafeBitCast(value, to: AXUIElement.self))
}

func copyAttributeValue(_ element: AXUIElement, _ attribute: CFString) -> (AXError, CFTypeRef?) {
  var value: CFTypeRef?
  let error = AXUIElementCopyAttributeValue(element, attribute, &value)
  return (error, value)
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

func copyWindowGeometry(_ element: AXUIElement) -> WindowGeometry {
  let position =
    copyCGPointAttribute(element, kAXPositionAttribute as CFString)
    .map {
      WindowPosition(x: Double($0.x), y: Double($0.y))
    }
  let size =
    copyCGSizeAttribute(element, kAXSizeAttribute as CFString)
    .map {
      WindowSize(width: Double($0.width), height: Double($0.height))
    }

  return WindowGeometry(position: position, size: size)
}

@MainActor
func copyFrontmostWindowGeometry(_ processIdentifier: pid_t) -> WindowGeometry? {
  let now = ProcessInfo.processInfo.systemUptime

  frontmostWindowGeometryCache = frontmostWindowGeometryCache.filter {
    now - $0.value.timestamp <= frontmostWindowGeometryCacheLifetime
  }

  if let entry = frontmostWindowGeometryCache[processIdentifier],
    now - entry.timestamp <= frontmostWindowGeometryCacheLifetime
  {
    return entry.geometry
  }

  guard
    let windowInfoList = CGWindowListCopyWindowInfo(
      [.optionOnScreenOnly, .excludeDesktopElements],
      kCGNullWindowID
    ) as? [[String: Any]]
  else {
    frontmostWindowGeometryCache[processIdentifier] = FrontmostWindowGeometryCacheEntry(
      geometry: nil,
      timestamp: now
    )
    return nil
  }

  for windowInfo in windowInfoList {
    guard let ownerPID = windowInfo[kCGWindowOwnerPID as String] as? pid_t,
      ownerPID == processIdentifier,
      let boundsDictionary = windowInfo[kCGWindowBounds as String] as? NSDictionary
    else {
      continue
    }

    let layer = windowInfo[kCGWindowLayer as String] as? Int ?? 0
    guard layer == 0 else {
      continue
    }

    var bounds = CGRect.zero
    guard CGRectMakeWithDictionaryRepresentation(boundsDictionary, &bounds) else {
      continue
    }

    guard bounds.width > 0, bounds.height > 0 else {
      continue
    }

    let result = WindowGeometry(
      position: WindowPosition(x: Double(bounds.origin.x), y: Double(bounds.origin.y)),
      size: WindowSize(width: Double(bounds.width), height: Double(bounds.height))
    )

    frontmostWindowGeometryCache[processIdentifier] = FrontmostWindowGeometryCacheEntry(
      geometry: result,
      timestamp: now
    )

    return result
  }

  frontmostWindowGeometryCache[processIdentifier] = FrontmostWindowGeometryCacheEntry(
    geometry: nil,
    timestamp: now
  )

  return nil
}

@MainActor
func copySnapshot(
  cachedApplication: FrontmostApplication?,
  handleProcessIdentifier: (pid_t?, DetectionSource) -> Void
) -> Snapshot {
  let systemWideElement = AXUIElementCreateSystemWide()
  let workspaceFrontmostApplication = NSWorkspace.shared.frontmostApplication
  let (_, applicationElement) = copyAXUIElementAttributeValue(
    systemWideElement,
    kAXFocusedApplicationAttribute as CFString
  )
  let (_, systemWideFocusedUIElement) = copyAXUIElementAttributeValue(
    systemWideElement,
    kAXFocusedUIElementAttribute as CFString
  )

  let resolvedApplication = resolveFrontmostApplication(
    cachedApplication: cachedApplication,
    workspaceFrontmostApplication: workspaceFrontmostApplication,
    applicationElement: applicationElement,
    systemWideFocusedUIElement: systemWideFocusedUIElement,
    handleProcessIdentifier: handleProcessIdentifier
  )

  let applicationFocusedUIElement: AXUIElement? =
    resolvedApplication?.processIdentifier
    .map(AXUIElementCreateApplication)
    .flatMap { applicationElement in
      let (_, focusedUIElement) = copyAXUIElementAttributeValue(
        applicationElement,
        kAXFocusedUIElementAttribute as CFString
      )
      return focusedUIElement
    }

  let focusedElement: FocusedUIElement?
  if let element = applicationFocusedUIElement ?? systemWideFocusedUIElement {
    focusedElement = FocusedUIElement(
      element,
      applicationElement: applicationElement,
      processIdentifier: resolvedApplication?.processIdentifier
    )
  } else if let processIdentifier = resolvedApplication?.processIdentifier {
    let fallbackWindowGeometry = copyFrontmostWindowGeometry(processIdentifier)

    if fallbackWindowGeometry?.isEmpty == false {
      focusedElement = FocusedUIElement(windowGeometry: fallbackWindowGeometry)
    } else {
      focusedElement = nil
    }
  } else {
    focusedElement = nil
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
let frontmostWindowGeometryCacheLifetime: TimeInterval = 0.5
