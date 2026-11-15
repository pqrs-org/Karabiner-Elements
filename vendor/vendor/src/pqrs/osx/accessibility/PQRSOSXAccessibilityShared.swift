// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

typealias AXAttributeReader = (AXUIElement, CFString) -> (AXError, CFTypeRef?)

public enum PQRSOSXAccessibility {
  public typealias MonitorCallback =
    @Sendable @convention(c) (
      Int32,
      UnsafePointer<pqrs_osx_accessibility_snapshot>?
    ) -> Void
}

struct AccessibilityMonitoringOptions: OptionSet, Sendable {
  let rawValue: UInt32

  static let frontmostApplication = Self(rawValue: 1 << 0)
  static let focusedUIElement = Self(rawValue: 1 << 1)
  static let focusedWindow = Self(rawValue: 1 << 2)

  init(rawValue: UInt32) {
    self.rawValue = rawValue
  }

  init(_ monitoring: pqrs_osx_accessibility_monitoring) {
    self = []
    if monitoring.frontmost_application {
      insert(.frontmostApplication)
    }
    if monitoring.focused_ui_element {
      insert(.focusedUIElement)
    }
    if monitoring.focused_window {
      insert(.focusedWindow)
    }
  }
}

enum DetectionSource: Int32, Sendable, Equatable {
  case none = 0
  case workspace = 1
  case axObserver = 2
}

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
  let windowTitle: String?
  private(set) var windowPosition: WindowPosition?
  private(set) var windowSize: WindowSize?
  // Window geometry is obtained from Accessibility whenever it provides a usable
  // position or size. Some applications expose neither value, so Core Graphics is
  // used as a fallback. This source records which path supplied the current
  // geometry; the monitor uses .coreGraphics to enable lightweight polling because
  // Core Graphics does not provide the move and resize notifications available
  // with .ax geometry.
  private(set) var windowGeometrySource: WindowGeometrySource

  init(windowGeometry: WindowGeometry?) {
    role = nil
    subrole = nil
    roleDescription = nil
    title = nil
    description = nil
    identifier = nil
    windowTitle = nil
    windowPosition = windowGeometry?.position
    windowSize = windowGeometry?.size
    windowGeometrySource = windowGeometry?.isEmpty == false ? .coreGraphics : .none
  }

  @MainActor
  init(
    element: AXUIElement,
    copyValue: AXAttributeReader = copyAttributeValue
  ) {
    role = copyNonEmptyStringAttribute(element, kAXRoleAttribute as CFString, copyValue: copyValue)
    subrole = copyAttribute(element, kAXSubroleAttribute as CFString, copyValue: copyValue)
    roleDescription = copyAttribute(
      element, kAXRoleDescriptionAttribute as CFString, copyValue: copyValue)
    title = copyAttribute(element, kAXTitleAttribute as CFString, copyValue: copyValue)
    description = copyAttribute(element, kAXDescriptionAttribute as CFString, copyValue: copyValue)
    identifier = copyAttribute(element, kAXIdentifierAttribute as CFString, copyValue: copyValue)
    windowTitle = nil
    windowPosition = nil
    windowSize = nil
    windowGeometrySource = .none
  }

  @MainActor
  init(
    windowElement: AXUIElement?,
    processIdentifier: pid_t?,
    copyValue: AXAttributeReader = copyAttributeValue,
    readFallbackWindowGeometry: @MainActor (pid_t) -> WindowGeometry? = copyFrontmostWindowGeometry
  ) {
    role = nil
    subrole = nil
    roleDescription = nil
    title = nil
    description = nil
    identifier = nil
    windowTitle = windowElement.flatMap {
      copyNonEmptyStringAttribute($0, kAXTitleAttribute as CFString, copyValue: copyValue)
    }

    let axWindowGeometry =
      windowElement
      .map { copyWindowGeometry($0, copyValue: copyValue) }
      .flatMap { $0.isEmpty ? nil : $0 }
    let fallbackWindowGeometry: WindowGeometry? =
      if axWindowGeometry == nil, let processIdentifier {
        readFallbackWindowGeometry(processIdentifier)
      } else {
        nil
      }
    let windowGeometry = axWindowGeometry ?? fallbackWindowGeometry
    windowPosition = windowGeometry?.position
    windowSize = windowGeometry?.size
    windowGeometrySource =
      axWindowGeometry != nil
      ? .ax : (fallbackWindowGeometry != nil ? .coreGraphics : .none)
  }

  func updatingCoreGraphicsWindowGeometry(_ windowGeometry: WindowGeometry?) -> Self {
    var result = self
    result.windowPosition = windowGeometry?.position
    result.windowSize = windowGeometry?.size
    result.windowGeometrySource = windowGeometry?.isEmpty == false ? .coreGraphics : .none
    return result
  }

  @MainActor
  init(
    _ context: FocusedUIElementAXContext,
    processIdentifier: pid_t?,
    // Dependency injection for testing.
    copyValue: AXAttributeReader = copyAttributeValue,
    // Dependency injection for testing.
    readFallbackWindowGeometry: @MainActor (pid_t) -> WindowGeometry? = copyFrontmostWindowGeometry
  ) {
    let element = context.element

    role = copyNonEmptyStringAttribute(element, kAXRoleAttribute as CFString, copyValue: copyValue)
    subrole = copyAttribute(element, kAXSubroleAttribute as CFString, copyValue: copyValue)
    roleDescription = copyAttribute(
      element, kAXRoleDescriptionAttribute as CFString, copyValue: copyValue)
    title = copyAttribute(element, kAXTitleAttribute as CFString, copyValue: copyValue)
    description = copyAttribute(element, kAXDescriptionAttribute as CFString, copyValue: copyValue)
    identifier = copyAttribute(element, kAXIdentifierAttribute as CFString, copyValue: copyValue)

    let windowElement = context.elementWindow ?? context.focusedWindow

    windowTitle = context.windowTitle

    let axWindowGeometry =
      windowElement
      .map { copyWindowGeometry($0, copyValue: copyValue) }
      .flatMap { $0.isEmpty ? nil : $0 }

    let fallbackWindowGeometry: WindowGeometry? =
      if axWindowGeometry == nil, let processIdentifier {
        readFallbackWindowGeometry(processIdentifier)
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

struct FocusedUIElementAXContext {
  let element: AXUIElement
  let topLevelUIElement: AXUIElement?
  let elementWindow: AXUIElement?
  let focusedWindow: AXUIElement?
  let windowTitle: String?
  let titleNotificationElements: [AXUIElement]

  init(
    element: AXUIElement,
    applicationElement: AXUIElement?,
    // Dependency injection for testing.
    copyValue: AXAttributeReader = copyAttributeValue
  ) {
    self.element = element
    let topLevelUIElement: AXUIElement? = copyAttribute(
      element,
      kAXTopLevelUIElementAttribute as CFString,
      copyValue: copyValue
    )
    let elementWindow: AXUIElement? = copyAttribute(
      element,
      kAXWindowAttribute as CFString,
      copyValue: copyValue
    )
    let focusedWindow: AXUIElement? = applicationElement.flatMap {
      copyAttribute($0, kAXFocusedWindowAttribute as CFString, copyValue: copyValue)
    }

    self.topLevelUIElement = topLevelUIElement
    self.elementWindow = elementWindow
    self.focusedWindow = focusedWindow

    let titleCandidates: [AXUIElement] = [topLevelUIElement, elementWindow, focusedWindow]
      .compactMap { $0 }
      .reduce(into: [AXUIElement]()) { result, element in
        if !result.contains(where: { CFEqual($0, element) }) {
          result.append(element)
        }
      }

    var windowTitle: String?
    var windowTitleElement: AXUIElement?
    for candidate in titleCandidates {
      if let title = copyNonEmptyStringAttribute(
        candidate, kAXTitleAttribute as CFString, copyValue: copyValue)
      {
        windowTitle = title
        windowTitleElement = candidate
        break
      }
    }

    self.windowTitle = windowTitle
    titleNotificationElements = windowTitleElement.map { [$0] } ?? titleCandidates
  }
}

struct Snapshot: Sendable, Equatable {
  let application: FrontmostApplication?
  let focusedUIElement: FocusedUIElement?
}

extension PQRSOSXAccessibility {
  struct FrontmostProcessIdentifiers: Sendable, Equatable {
    let axPid: pid_t?
    let workspacePid: pid_t?
  }

  struct FrontmostProcessIdentifierResolution: Sendable, Equatable {
    let processIdentifier: pid_t?
    let detectionSource: DetectionSource
    let sourceProcessIdentifiers: FrontmostProcessIdentifiers
  }

  static func makeFrontmostProcessIdentifiers(
    applicationElement: AXUIElement?,
    systemWideFocusedUIElement: AXUIElement?,
    workspaceFrontmostApplication: NSRunningApplication?
  ) -> FrontmostProcessIdentifiers {
    FrontmostProcessIdentifiers(
      axPid:
        applicationElement
        .flatMap(copyPid(_:))
        ?? systemWideFocusedUIElement
        .flatMap(copyPid(_:)),
      workspacePid: workspaceFrontmostApplication?.processIdentifier
    )
  }

  @MainActor
  static func copyFrontmostProcessIdentifiers(
    monitoringOptions: AccessibilityMonitoringOptions = [.frontmostApplication]
  ) -> FrontmostProcessIdentifiers {
    guard monitoringOptions.contains(.frontmostApplication) else {
      return makeFrontmostProcessIdentifiers(
        applicationElement: nil,
        systemWideFocusedUIElement: nil,
        workspaceFrontmostApplication: NSWorkspace.shared.frontmostApplication
      )
    }
    let systemWideElement = AXUIElementCreateSystemWide()
    let (_, applicationElement) = copyAXUIElementAttributeValue(
      systemWideElement,
      kAXFocusedApplicationAttribute as CFString
    )
    let (_, systemWideFocusedUIElement) = copyAXUIElementAttributeValue(
      systemWideElement,
      kAXFocusedUIElementAttribute as CFString
    )

    return makeFrontmostProcessIdentifiers(
      applicationElement: applicationElement,
      systemWideFocusedUIElement: systemWideFocusedUIElement,
      workspaceFrontmostApplication: NSWorkspace.shared.frontmostApplication
    )
  }
}

@MainActor
func resolveFrontmostApplication(
  cachedApplication: FrontmostApplication?,
  workspaceFrontmostApplication: NSRunningApplication?,
  resolution: PQRSOSXAccessibility.FrontmostProcessIdentifierResolution,
  handleProcessIdentifier: (pid_t?, DetectionSource) -> Void
) -> FrontmostApplication? {
  switch resolution.detectionSource {
  case .workspace:
    handleProcessIdentifier(
      resolution.sourceProcessIdentifiers.workspacePid,
      .workspace
    )
  case .axObserver:
    // An AX-detected result means AX and Workspace disagree and the AX PID was
    // selected; it does not mean that Workspace has no PID. Register both sides
    // with their actual sources so the observation controller can keep the
    // Workspace-known process classified normally while attaching an AXObserver
    // to the AX-only process. This also lets later convergence or termination
    // remove the extra AXObserver through the normal synchronization path.
    handleProcessIdentifier(
      resolution.sourceProcessIdentifiers.workspacePid,
      .workspace
    )
    handleProcessIdentifier(
      resolution.sourceProcessIdentifiers.axPid,
      .axObserver
    )
  case .none:
    break
  }

  return if let processIdentifier = resolution.processIdentifier {
    if cachedApplication?.processIdentifier == processIdentifier,
      cachedApplication?.detectionSource == resolution.detectionSource
    {
      cachedApplication
    } else {
      FrontmostApplication(
        processIdentifier: processIdentifier,
        detectionSource: resolution.detectionSource
      )
    }
  } else {
    FrontmostApplication(
      workspaceFrontmostApplication,
      detectionSource: resolution.detectionSource
    )
  }
}

func copyAttribute<T>(
  _ element: AXUIElement,
  _ attribute: CFString,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> T? {
  let (error, value) = copyValue(element, attribute)
  guard error == .success, let value else {
    return nil
  }

  return value as? T
}

func copyNonEmptyStringAttribute(
  _ element: AXUIElement,
  _ attribute: CFString,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> String? {
  guard let value: String = copyAttribute(element, attribute, copyValue: copyValue), !value.isEmpty
  else {
    return nil
  }

  return value
}

func copyAXUIElementAttributeValue(
  _ element: AXUIElement,
  _ attribute: CFString,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> (
  AXError, AXUIElement?
) {
  let (error, value) = copyValue(element, attribute)
  guard error == .success, let value else {
    return (error, nil)
  }

  guard CFGetTypeID(value) == AXUIElementGetTypeID() else {
    return (error, nil)
  }

  return (error, unsafeBitCast(value, to: AXUIElement.self))
}

func copyApplicationFocusedUIElement(
  _ applicationElement: AXUIElement,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> AXUIElement? {
  // Chrome activates its native Accessibility interface when the application's
  // role is requested. Reading the focused element's role cannot initialise an
  // interface that has not yet supplied a focused element.
  _ = copyValue(applicationElement, kAXRoleAttribute as CFString)

  // The role request is best effort: applications may still expose focus when
  // their application role is unavailable. Initialisation may also complete
  // asynchronously, in which case the monitor retries on later polling ticks.
  let (_, focusedUIElement) = copyAXUIElementAttributeValue(
    applicationElement,
    kAXFocusedUIElementAttribute as CFString,
    copyValue: copyValue
  )
  return focusedUIElement
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

func copyCGPointAttribute(
  _ element: AXUIElement,
  _ attribute: CFString,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> CGPoint? {
  guard let value: AXValue = copyAttribute(element, attribute, copyValue: copyValue) else {
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

func copyCGSizeAttribute(
  _ element: AXUIElement,
  _ attribute: CFString,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> CGSize? {
  guard let value: AXValue = copyAttribute(element, attribute, copyValue: copyValue) else {
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

func copyWindowGeometry(
  _ element: AXUIElement,
  // Dependency injection for testing.
  copyValue: AXAttributeReader = copyAttributeValue
) -> WindowGeometry {
  let position =
    copyCGPointAttribute(element, kAXPositionAttribute as CFString, copyValue: copyValue)
    .map {
      WindowPosition(x: Double($0.x), y: Double($0.y))
    }
  let size =
    copyCGSizeAttribute(element, kAXSizeAttribute as CFString, copyValue: copyValue)
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

struct SnapshotCopyResult {
  let snapshot: Snapshot
  let titleNotificationElements: [AXUIElement]
}

// Dependency injection for testing.
struct SnapshotCopyDependencies {
  let readWorkspaceFrontmostApplication: @MainActor () -> NSRunningApplication?
  let createSystemWideElement: () -> AXUIElement
  let createApplicationElement: (pid_t) -> AXUIElement
  let copyValue: AXAttributeReader
  let readFallbackWindowGeometry: @MainActor (pid_t) -> WindowGeometry?

  static let live = Self(
    readWorkspaceFrontmostApplication: { NSWorkspace.shared.frontmostApplication },
    createSystemWideElement: AXUIElementCreateSystemWide,
    createApplicationElement: AXUIElementCreateApplication,
    copyValue: copyAttributeValue,
    readFallbackWindowGeometry: copyFrontmostWindowGeometry
  )
}

@MainActor
func copySnapshot(
  cachedApplication: FrontmostApplication?,
  monitoringOptions: AccessibilityMonitoringOptions,
  resolveProcessIdentifiers: (
    PQRSOSXAccessibility.FrontmostProcessIdentifiers
  ) -> PQRSOSXAccessibility.FrontmostProcessIdentifierResolution,
  handleProcessIdentifier: (pid_t?, DetectionSource) -> Void
) -> SnapshotCopyResult {
  copySnapshot(
    cachedApplication: cachedApplication,
    monitoringOptions: monitoringOptions,
    resolveProcessIdentifiers: resolveProcessIdentifiers,
    handleProcessIdentifier: handleProcessIdentifier,
    dependencies: .live
  )
}

@MainActor
func copySnapshot(
  cachedApplication: FrontmostApplication?,
  monitoringOptions: AccessibilityMonitoringOptions,
  resolveProcessIdentifiers: (
    PQRSOSXAccessibility.FrontmostProcessIdentifiers
  ) -> PQRSOSXAccessibility.FrontmostProcessIdentifierResolution,
  handleProcessIdentifier: (pid_t?, DetectionSource) -> Void,
  // Dependency injection for testing.
  dependencies: SnapshotCopyDependencies
) -> SnapshotCopyResult {
  let workspaceFrontmostApplication = dependencies.readWorkspaceFrontmostApplication()
  let usesAXFrontmostApplication = monitoringOptions.contains(.frontmostApplication)
  let readsFocusedUIElement = monitoringOptions.contains(.focusedUIElement)
  let readsFocusedWindow = monitoringOptions.contains(.focusedWindow)
  let systemWideElement =
    usesAXFrontmostApplication
    ? dependencies.createSystemWideElement() : nil
  let applicationElement =
    systemWideElement.flatMap {
      copyAXUIElementAttributeValue(
        $0,
        kAXFocusedApplicationAttribute as CFString,
        copyValue: dependencies.copyValue
      ).1
    }
  let systemWideFocusedUIElement =
    systemWideElement.flatMap {
      copyAXUIElementAttributeValue(
        $0,
        kAXFocusedUIElementAttribute as CFString,
        copyValue: dependencies.copyValue
      ).1
    }

  let processIdentifiers = PQRSOSXAccessibility.makeFrontmostProcessIdentifiers(
    applicationElement: applicationElement,
    systemWideFocusedUIElement: systemWideFocusedUIElement,
    workspaceFrontmostApplication: workspaceFrontmostApplication
  )

  let resolvedApplication = resolveFrontmostApplication(
    cachedApplication: cachedApplication,
    workspaceFrontmostApplication: workspaceFrontmostApplication,
    resolution: resolveProcessIdentifiers(processIdentifiers),
    handleProcessIdentifier: handleProcessIdentifier
  )

  let resolvedApplicationElement =
    readsFocusedUIElement || readsFocusedWindow
    ? resolvedApplication?.processIdentifier
      .map(dependencies.createApplicationElement)
    : nil

  let applicationFocusedUIElement: AXUIElement? =
    readsFocusedUIElement
    ? resolvedApplicationElement.flatMap {
      copyApplicationFocusedUIElement($0, copyValue: dependencies.copyValue)
    }
    : nil

  let matchingSystemWideFocusedUIElement: AXUIElement?
  if let element = systemWideFocusedUIElement,
    let processIdentifier = resolvedApplication?.processIdentifier,
    copyPid(element) == processIdentifier
  {
    matchingSystemWideFocusedUIElement = element
  } else {
    matchingSystemWideFocusedUIElement = nil
  }

  let focusedElement: FocusedUIElement?
  let titleNotificationElements: [AXUIElement]
  if readsFocusedUIElement,
    let element = applicationFocusedUIElement ?? matchingSystemWideFocusedUIElement
  {
    if readsFocusedWindow {
      let context = FocusedUIElementAXContext(
        element: element,
        applicationElement: resolvedApplicationElement ?? applicationElement,
        copyValue: dependencies.copyValue
      )
      focusedElement = FocusedUIElement(
        context,
        processIdentifier: resolvedApplication?.processIdentifier,
        copyValue: dependencies.copyValue,
        readFallbackWindowGeometry: dependencies.readFallbackWindowGeometry
      )
      titleNotificationElements = context.titleNotificationElements
    } else {
      focusedElement = FocusedUIElement(element: element, copyValue: dependencies.copyValue)
      titleNotificationElements = []
    }
  } else if readsFocusedWindow {
    let focusedWindow: AXUIElement? = resolvedApplicationElement.flatMap {
      copyAttribute(
        $0,
        kAXFocusedWindowAttribute as CFString,
        copyValue: dependencies.copyValue
      )
    }
    focusedElement = FocusedUIElement(
      windowElement: focusedWindow,
      processIdentifier: resolvedApplication?.processIdentifier,
      copyValue: dependencies.copyValue,
      readFallbackWindowGeometry: dependencies.readFallbackWindowGeometry
    )
    titleNotificationElements = focusedWindow.map { [$0] } ?? []
  } else {
    focusedElement = nil
    titleNotificationElements = []
  }

  return SnapshotCopyResult(
    snapshot: Snapshot(
      application: resolvedApplication,
      focusedUIElement: focusedElement
    ),
    titleNotificationElements: titleNotificationElements
  )
}

// Some transient system UIs such as Spotlight do not reliably surface through
// app-activation or focused-element notifications, so keep a modest fallback poll.
let fallbackPollingInterval = Duration.milliseconds(500)
let focusedUIElementRoleRecoveryMaxPolls = 10
let staleProcessCleanupInterval = Duration.seconds(10)
let frontmostWindowGeometryCacheLifetime: TimeInterval = 0.5
