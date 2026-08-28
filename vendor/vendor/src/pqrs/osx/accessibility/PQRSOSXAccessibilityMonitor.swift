// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

// AX notifications for one UI change often arrive together. Delay their shared
// refresh briefly so the whole burst can be handled by one snapshot.
private let accessibilityNotificationRefreshCoalescingInterval = Duration.milliseconds(10)

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

private enum SnapshotCStringField: Int, CaseIterable {
  case applicationName
  case bundleIdentifier
  case bundlePath
  case filePath
  case role
  case subrole
  case roleDescription
  case title
  case description
  case identifier
  case windowTitle
}

private struct SnapshotCStringPointers {
  let values: [UnsafePointer<CChar>?]

  subscript(_ field: SnapshotCStringField) -> UnsafePointer<CChar>? {
    values[field.rawValue]
  }
}

private struct SnapshotCStringValues {
  private var values = [String?](
    repeating: nil,
    count: SnapshotCStringField.allCases.count
  )

  func setting(_ field: SnapshotCStringField, to value: String?) -> Self {
    var result = self
    result.values[field.rawValue] = value
    return result
  }

  func withUnsafePointers<Result>(
    _ body: (SnapshotCStringPointers) -> Result
  ) -> Result {
    var pointers = [UnsafePointer<CChar>?](repeating: nil, count: values.count)

    // Each recursive call remains inside the preceding withCString scope, so
    // all pointers remain valid until body returns.
    func invoke(_ index: Int) -> Result {
      guard index < values.count else {
        return body(SnapshotCStringPointers(values: pointers))
      }

      return withOptionalCString(values[index]) { pointer in
        pointers[index] = pointer
        return invoke(index + 1)
      }
    }

    return invoke(0)
  }
}

extension PQRSOSXAccessibility {
  struct AccessibilityNotificationRefreshScheduleState {
    private var scheduled = false

    // Returns true only when the caller must create the delayed refresh task.
    mutating func schedule() -> Bool {
      guard !scheduled else {
        return false
      }

      scheduled = true
      return true
    }

    mutating func reset() {
      scheduled = false
    }
  }

  // Serializes refresh requests and coalesces requests made while a refresh is
  // already in progress. The caller owns the actual snapshot evaluation loop.
  struct RefreshRequestState {
    private var refreshInFlight = false
    private var refreshPending = false
    private var forcePending = false

    // Returns true only for the request that must start the evaluation loop.
    mutating func request(force: Bool) -> Bool {
      forcePending = forcePending || force
      refreshPending = true

      guard !refreshInFlight else {
        return false
      }

      refreshInFlight = true
      return true
    }

    // Returns the next coalesced force value. Returning nil drains the queue and
    // releases the in-flight state so a later request can start a new loop.
    mutating func takePendingForce() -> Bool? {
      guard refreshPending else {
        refreshInFlight = false
        return nil
      }

      refreshPending = false
      let force = forcePending
      forcePending = false
      return force
    }
  }

  private struct ProcessIdentifierObservation {
    private(set) var changedGeneration: UInt64 = 0
    private var hasObservedValue = false
    private var value: pid_t?

    mutating func observe(_ value: pid_t?, generation: UInt64) -> Bool {
      guard !hasObservedValue || self.value != value else {
        return false
      }

      hasObservedValue = true
      self.value = value
      changedGeneration = generation
      return true
    }
  }

  // Tracks AX and Workspace PID changes and resolves the frontmost application:
  //
  // AX and Workspace can change at different times. An unbundled GUI application
  // may update only Workspace, while Spotlight may update only AX. The generation
  // in which each PID last changed is recorded to resolve such disagreements.
  // Sources changed in the same observation receive the same generation.
  //
  // - Prefer the source whose PID changed in a later observation generation.
  // - If both changed in the same generation, prefer AX. AX-only state occurs in
  //   ordinary macOS behavior such as Spotlight, while Workspace-only state is
  //   mainly needed for exceptional cases such as an unbundled GUI executable.
  //   Consequently, if the monitor starts while an unbundled GUI application is
  //   already frontmost and AX still reports another application, AX is selected
  //   until either source changes.
  // - Fall back to the other source when the preferred source has no valid PID.
  // - Classify a PID also reported by Workspace as Workspace-detected; a selected
  //   AX-only PID requires AXObserver management.
  struct FrontmostProcessIdentifierObservations {
    private var generation: UInt64 = 0
    private var ax = ProcessIdentifierObservation()
    private var workspace = ProcessIdentifierObservation()

    mutating func observe(
      _ processIdentifiers: FrontmostProcessIdentifiers
    ) -> (
      changed: Bool,
      resolution: FrontmostProcessIdentifierResolution
    ) {
      generation &+= 1

      let axChanged = ax.observe(
        processIdentifiers.axPid,
        generation: generation
      )
      let workspaceChanged = workspace.observe(
        processIdentifiers.workspacePid,
        generation: generation
      )

      let preferredDetectionSource: DetectionSource? =
        if processIdentifiers.axPid == processIdentifiers.workspacePid {
          nil
        } else if workspace.changedGeneration > ax.changedGeneration {
          .workspace
        } else {
          .axObserver
        }

      let validAXProcessIdentifier =
        processIdentifiers.axPid.flatMap { $0 == 0 ? nil : $0 }
      let validWorkspaceProcessIdentifier =
        processIdentifiers.workspacePid.flatMap { $0 == 0 ? nil : $0 }

      let processIdentifier: pid_t?
      switch preferredDetectionSource {
      case .some(.workspace):
        processIdentifier = validWorkspaceProcessIdentifier ?? validAXProcessIdentifier
      case .some(.axObserver), .some(.none), nil:
        processIdentifier = validAXProcessIdentifier ?? validWorkspaceProcessIdentifier
      }

      let detectionSource: DetectionSource =
        if processIdentifier == nil {
          .none
        } else if processIdentifier == validWorkspaceProcessIdentifier {
          .workspace
        } else {
          .axObserver
        }

      return (
        axChanged || workspaceChanged,
        FrontmostProcessIdentifierResolution(
          processIdentifier: processIdentifier,
          detectionSource: detectionSource,
          sourceProcessIdentifiers: processIdentifiers
        )
      )
    }
  }
}

extension PQRSOSXAccessibility {
  @MainActor
  final class Monitor {
    static let shared = Monitor()

    private var callback: MonitorCallback?
    private var accessibilityNotificationRefreshTask: Task<Void, Never>?
    private var accessibilityNotificationRefreshScheduleState =
      PQRSOSXAccessibility.AccessibilityNotificationRefreshScheduleState()
    private var fallbackPollingTask: Task<Void, Never>?
    private var staleProcessCleanupTask: Task<Void, Never>?
    private var observationController: ObservationController?
    private var lastSnapshot = Snapshot(application: nil, focusedUIElement: nil)
    private var processIdentifierObservations =
      PQRSOSXAccessibility.FrontmostProcessIdentifierObservations()
    private var refreshRequestState = PQRSOSXAccessibility.RefreshRequestState()
    private var callbackGeneration = 0

    func setCallback(_ callback: @escaping MonitorCallback) {
      if self.callback == nil {
        callbackGeneration += 1
      }

      self.callback = callback

      if observationController == nil {
        observationController = ObservationController()
      }

      let callbackGeneration = callbackGeneration

      observationController?.start(callbackGeneration: callbackGeneration)

      if fallbackPollingTask == nil {
        let callbackGeneration = callbackGeneration
        fallbackPollingTask = Task {
          await listenLoop(callbackGeneration: callbackGeneration)
        }
      }

      if staleProcessCleanupTask == nil {
        let callbackGeneration = callbackGeneration
        staleProcessCleanupTask = Task {
          await cleanupLoop(callbackGeneration: callbackGeneration)
        }
      }
    }

    func unsetCallback() {
      callbackGeneration += 1
      callback = nil
      cancelScheduledAccessibilityNotificationRefresh()
      fallbackPollingTask?.cancel()
      fallbackPollingTask = nil
      staleProcessCleanupTask?.cancel()
      staleProcessCleanupTask = nil
      lastSnapshot = Snapshot(application: nil, focusedUIElement: nil)
      processIdentifierObservations = PQRSOSXAccessibility.FrontmostProcessIdentifierObservations()
      refreshRequestState = PQRSOSXAccessibility.RefreshRequestState()

      let observationController = observationController
      self.observationController = nil
      observationController?.stop()
      frontmostWindowGeometryCache.removeAll()
    }

    func trigger() {
      requestRefresh(force: true)
    }

    private func listenLoop(callbackGeneration: Int) async {
      while !Task.isCancelled {
        do {
          try await Task.sleep(for: fallbackPollingInterval)
        } catch {
          break
        }

        guard isCurrentCallbackGeneration(callbackGeneration) else {
          break
        }

        refreshIfPollingNeedsSnapshot()
      }
    }

    private func cleanupLoop(callbackGeneration: Int) async {
      while !Task.isCancelled {
        do {
          try await Task.sleep(for: staleProcessCleanupInterval)
        } catch {
          break
        }

        guard isCurrentCallbackGeneration(callbackGeneration) else {
          break
        }

        observationController?.pruneStaleProcessIdentifiers()
      }
    }

    // Notifications, polling, and trigger() all converge here. If a refresh is
    // already running, record the pending request instead of evaluating another
    // snapshot recursively. The loop then evaluates the latest state serially and
    // coalesces multiple requests into as few snapshots as possible.
    func requestRefresh(force: Bool) {
      guard callback != nil else {
        return
      }

      // Any immediate refresh also covers pending AX notifications.
      cancelScheduledAccessibilityNotificationRefresh()

      guard refreshRequestState.request(force: force) else {
        return
      }

      while let force = refreshRequestState.takePendingForce() {
        let cachedApplication = lastSnapshot.application
        let observationController = observationController
        let result = copySnapshot(
          cachedApplication: cachedApplication,
          resolveProcessIdentifiers: { processIdentifiers in
            self.processIdentifierObservations.observe(processIdentifiers)
              .resolution
          },
          handleProcessIdentifier: { processIdentifier, detectionSource in
            observationController?.registerProcessIdentifier(
              processIdentifier,
              detectionSource: detectionSource
            )
          }
        )
        observationController?.syncObservers(
          frontmostProcessIdentifier: result.snapshot.application?.processIdentifier,
          titleNotificationElements: result.titleNotificationElements
        )

        if force || result.snapshot != lastSnapshot {
          commitSnapshotAndEmit(result.snapshot, force: force)
        }
      }

    }

    func requestRefresh(force: Bool, callbackGeneration: Int) {
      guard isCurrentCallbackGeneration(callbackGeneration) else {
        return
      }

      requestRefresh(force: force)
    }

    func scheduleAccessibilityNotificationRefresh(callbackGeneration: Int) {
      guard isCurrentCallbackGeneration(callbackGeneration) else {
        return
      }

      guard accessibilityNotificationRefreshScheduleState.schedule() else {
        return
      }

      accessibilityNotificationRefreshTask = Task { @MainActor [weak self] in
        do {
          try await Task.sleep(for: accessibilityNotificationRefreshCoalescingInterval)
        } catch {
          return
        }

        guard let self else {
          return
        }

        self.accessibilityNotificationRefreshScheduleState.reset()
        self.accessibilityNotificationRefreshTask = nil
        self.requestRefresh(
          force: false,
          callbackGeneration: callbackGeneration
        )
      }
    }

    private func isCurrentCallbackGeneration(_ callbackGeneration: Int) -> Bool {
      self.callbackGeneration == callbackGeneration && callback != nil
    }

    private func cancelScheduledAccessibilityNotificationRefresh() {
      accessibilityNotificationRefreshScheduleState.reset()
      accessibilityNotificationRefreshTask?.cancel()
      accessibilityNotificationRefreshTask = nil
    }

    // In general, information about the currently focused application can be obtained through the following mechanisms:
    //
    // - Application switches can be detected via NSWorkspace.didActivateApplicationNotification.
    // - Window position and size changes can be detected via Accessibility notifications.
    //
    // However, some applications do not work with these mechanisms. Specifically, there are three categories:
    //
    // - Applications such as Spotlight, where NSWorkspace.didActivateApplicationNotification is not delivered.
    //   (Once the application has been detected at least once and AXObserverAddNotification has been registered for its process,
    //   subsequent detections can be made via kAXFocusedUIElementChangedNotification.)
    // - Applications such as Google Chrome and Electron-based apps that do not provide sufficient information through Accessibility.
    //   (window position and size changes cannot be obtained from notifications).
    // - Unbundled GUI applications, such as some Rust GUI applications launched
    //   directly as a single executable, that update NSWorkspace.frontmostApplication without delivering
    //   NSWorkspace.didActivateApplicationNotification, while Accessibility may
    //   continue to report the previously focused application.
    //
    // For these applications, notification-based detection is not sufficient, so polling is required.
    // In particular, Spotlight-style application switches can be missed unless polling runs at a fairly high frequency.
    // For that reason, polling is performed every 500 ms.
    //
    // Because this polling needs to stay lightweight, it updates state only when
    // necessary. More specifically, an update is performed in the following cases:
    //
    // - When polling detects a change in either the Accessibility or NSWorkspace
    //   frontmost application. These sources are checked separately because an
    //   unbundled GUI application may update only NSWorkspace, whereas a transient
    //   system UI such as Spotlight may update only Accessibility.
    // - When lightweight Core Graphics polling detects a geometry change for a
    //   window whose position or size could not be obtained through Accessibility.
    // - When lightweight title polling detects a change for a window that does not
    //   support title-change notifications.
    private func refreshIfPollingNeedsSnapshot() {
      let processIdentifiers = PQRSOSXAccessibility.copyFrontmostProcessIdentifiers()
      // AX and NSWorkspace do not always change together. Observe both sources so
      // an application switch reported by either one schedules a full snapshot.
      let applicationChanged = processIdentifierObservations.observe(processIdentifiers).changed
      if applicationChanged {
        requestRefresh(force: false)
        return
      }

      // Some applications do not expose window geometry through Accessibility.
      // Compare only the Core Graphics geometry on each polling tick and avoid
      // querying the remaining snapshot fields when it actually changes.
      var geometryUpdatedSnapshot: Snapshot?
      if let applicationProcessIdentifier = lastSnapshot.application?.processIdentifier,
        let focusedUIElement = lastSnapshot.focusedUIElement,
        focusedUIElement.windowGeometrySource == .coreGraphics
      {
        let latestWindowGeometry = copyFrontmostWindowGeometry(applicationProcessIdentifier)
        let currentWindowGeometry = WindowGeometry(
          position: focusedUIElement.windowPosition,
          size: focusedUIElement.windowSize
        )
        if latestWindowGeometry != currentWindowGeometry {
          geometryUpdatedSnapshot = Snapshot(
            application: lastSnapshot.application,
            focusedUIElement: focusedUIElement.updatingCoreGraphicsWindowGeometry(
              latestWindowGeometry
            )
          )
        }
      }

      // If title notifications are unavailable, compare only AXTitle here and
      // avoid building a full snapshot until the value actually changes. If title
      // and geometry change in the same tick, the full refresh also incorporates
      // the new geometry from the Core Graphics cache.
      if observationController?.windowTitleNeedsRefresh(
        currentWindowTitle: lastSnapshot.focusedUIElement?.windowTitle
      ) == true {
        requestRefresh(force: false)
        return
      }

      if let geometryUpdatedSnapshot {
        commitSnapshotAndEmit(geometryUpdatedSnapshot, force: false)
      }
    }

    private func commitSnapshotAndEmit(_ snapshot: Snapshot, force: Bool) {
      lastSnapshot = snapshot

      guard let callback else {
        return
      }

      let application = snapshot.application
      let element = snapshot.focusedUIElement

      SnapshotCStringValues()
        .setting(.applicationName, to: application?.name)
        .setting(.bundleIdentifier, to: application?.bundleIdentifier)
        .setting(.bundlePath, to: application?.bundlePath)
        .setting(.filePath, to: application?.filePath)
        .setting(.role, to: element?.role)
        .setting(.subrole, to: element?.subrole)
        .setting(.roleDescription, to: element?.roleDescription)
        .setting(.title, to: element?.title)
        .setting(.description, to: element?.description)
        .setting(.identifier, to: element?.identifier)
        .setting(.windowTitle, to: element?.windowTitle)
        .withUnsafePointers { strings in
          var cSnapshot = pqrs_osx_accessibility_snapshot(
            application_name: strings[.applicationName],
            bundle_identifier: strings[.bundleIdentifier],
            bundle_path: strings[.bundlePath],
            file_path: strings[.filePath],
            pid: application?.processIdentifier ?? 0,
            application_detection_source: application?.detectionSource.rawValue ?? 0,
            role: strings[.role],
            subrole: strings[.subrole],
            role_description: strings[.roleDescription],
            title: strings[.title],
            description: strings[.description],
            identifier: strings[.identifier],
            window_title: strings[.windowTitle],
            has_window_position: element?.windowPosition == nil ? 0 : 1,
            window_position_x: element?.windowPosition?.x ?? 0,
            window_position_y: element?.windowPosition?.y ?? 0,
            has_window_size: element?.windowSize == nil ? 0 : 1,
            window_size_width: element?.windowSize?.width ?? 0,
            window_size_height: element?.windowSize?.height ?? 0
          )

          withUnsafePointer(to: &cSnapshot) { cSnapshot in
            callback(force ? 1 : 0, cSnapshot)
          }
        }
    }
  }
}
