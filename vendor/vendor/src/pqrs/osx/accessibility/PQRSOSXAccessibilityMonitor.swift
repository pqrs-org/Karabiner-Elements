// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

extension PQRSOSXAccessibility {
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

      guard refreshRequestState.request(force: force) else {
        return
      }

      while let force = refreshRequestState.takePendingForce() {
        let cachedApplication = lastSnapshot.application
        let observationController = observationController
        let snapshot = copySnapshot(
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
          frontmostProcessIdentifier: snapshot.application?.processIdentifier
        )

        if force || snapshot != lastSnapshot {
          commitSnapshotAndEmit(snapshot, force: force)
        }
      }

    }

    func requestRefresh(force: Bool, callbackGeneration: Int) {
      guard isCurrentCallbackGeneration(callbackGeneration) else {
        return
      }

      requestRefresh(force: force)
    }

    private func isCurrentCallbackGeneration(_ callbackGeneration: Int) -> Bool {
      self.callbackGeneration == callbackGeneration && callback != nil
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
    // Because this polling needs to stay lightweight, requestRefresh is called only when necessary.
    // More specifically, requestRefresh is triggered only in the following cases:
    //
    // - When polling detects a change in either the Accessibility or NSWorkspace
    //   frontmost application. These sources are checked separately because an
    //   unbundled GUI application may update only NSWorkspace, whereas a transient
    //   system UI such as Spotlight may update only Accessibility.
    // - When the current application's window position or size could not be obtained through the Accessibility API.
    //   (requestRefresh is called in order to fetch the latest window position and size.)
    private func refreshIfPollingNeedsSnapshot() {
      let processIdentifiers = PQRSOSXAccessibility.copyFrontmostProcessIdentifiers()
      // AX and NSWorkspace do not always change together. Observe both sources so
      // an application switch reported by either one schedules a full snapshot.
      let applicationChanged = processIdentifierObservations.observe(processIdentifiers).changed

      // Some applications do not expose window geometry through Accessibility.
      // Their geometry comes from Core Graphics, which has no corresponding AX
      // move/resize notification, so it must be refreshed on every polling tick.
      let needsGeometryPolling =
        lastSnapshot.focusedUIElement?.windowGeometrySource == .coreGraphics

      if applicationChanged || needsGeometryPolling {
        requestRefresh(force: false)
      }
    }

    private func commitSnapshotAndEmit(_ snapshot: Snapshot, force: Bool) {
      lastSnapshot = snapshot

      guard let callback else {
        return
      }

      withOptionalCString(snapshot.application?.name) { applicationName in
        withOptionalCString(snapshot.application?.bundleIdentifier) { bundleIdentifier in
          withOptionalCString(snapshot.application?.bundlePath) { bundlePath in
            withOptionalCString(snapshot.application?.filePath) { filePath in
              withOptionalCString(snapshot.focusedUIElement?.role) { role in
                withOptionalCString(snapshot.focusedUIElement?.subrole) { subrole in
                  withOptionalCString(snapshot.focusedUIElement?.roleDescription) {
                    roleDescription in
                    withOptionalCString(snapshot.focusedUIElement?.title) { title in
                      withOptionalCString(snapshot.focusedUIElement?.description) { description in
                        withOptionalCString(snapshot.focusedUIElement?.identifier) { identifier in
                          var cSnapshot = pqrs_osx_accessibility_snapshot(
                            application_name: applicationName,
                            bundle_identifier: bundleIdentifier,
                            bundle_path: bundlePath,
                            file_path: filePath,
                            pid: snapshot.application?.processIdentifier ?? 0,
                            application_detection_source: snapshot.application?.detectionSource
                              .rawValue ?? 0,
                            role: role,
                            subrole: subrole,
                            role_description: roleDescription,
                            title: title,
                            description: description,
                            identifier: identifier,
                            has_window_position: snapshot.focusedUIElement?.windowPosition == nil
                              ? 0 : 1,
                            window_position_x: snapshot.focusedUIElement?.windowPosition?.x ?? 0,
                            window_position_y: snapshot.focusedUIElement?.windowPosition?.y ?? 0,
                            has_window_size: snapshot.focusedUIElement?.windowSize == nil ? 0 : 1,
                            window_size_width: snapshot.focusedUIElement?.windowSize?.width ?? 0,
                            window_size_height: snapshot.focusedUIElement?.windowSize?.height ?? 0
                          )

                          withUnsafePointer(to: &cSnapshot) { cSnapshotPointer in
                            callback(force ? 1 : 0, cSnapshotPointer)
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
  }
}
