// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

actor PQRSOSXAccessibilityMonitor {
  static let shared = PQRSOSXAccessibilityMonitor()

  private var callback: PQRSOSXAccessibilityMonitorCallback?
  private var fallbackPollingTask: Task<Void, Never>?
  private var staleProcessCleanupTask: Task<Void, Never>?
  private var observationController: PQRSOSXAccessibilityObservationController?
  private var lastSnapshot = Snapshot(application: nil, focusedUIElement: nil)
  private var refreshInFlight = false
  private var refreshPending = false
  private var forcePending = false

  func setCallback(_ callback: @escaping PQRSOSXAccessibilityMonitorCallback) async {
    self.callback = callback

    if observationController == nil {
      observationController = await MainActor.run {
        PQRSOSXAccessibilityObservationController()
      }
    }

    let observationController = observationController
    await MainActor.run {
      observationController?.start()
    }

    if fallbackPollingTask == nil {
      fallbackPollingTask = Task {
        await listenLoop()
      }
    }

    if staleProcessCleanupTask == nil {
      staleProcessCleanupTask = Task {
        await cleanupLoop()
      }
    }
  }

  func unsetCallback() async {
    callback = nil
    fallbackPollingTask?.cancel()
    fallbackPollingTask = nil
    staleProcessCleanupTask?.cancel()
    staleProcessCleanupTask = nil
    lastSnapshot = Snapshot(application: nil, focusedUIElement: nil)
    refreshInFlight = false
    refreshPending = false
    forcePending = false

    let observationController = observationController
    self.observationController = nil
    await MainActor.run {
      observationController?.stop()
    }
  }

  func asyncTrigger() async {
    await requestRefresh(force: true)
  }

  private func listenLoop() async {
    while !Task.isCancelled {
      do {
        try await Task.sleep(for: fallbackPollingInterval)
      } catch {
        break
      }

      await refreshIfPollingNeedsSnapshot()
    }
  }

  private func cleanupLoop() async {
    while !Task.isCancelled {
      do {
        try await Task.sleep(for: staleProcessCleanupInterval)
      } catch {
        break
      }

      let observationController = observationController
      await MainActor.run {
        observationController?.pruneStaleProcessIdentifiers()
      }
    }
  }

  func requestRefresh(force: Bool) async {
    forcePending = forcePending || force
    refreshPending = true

    guard !refreshInFlight else {
      return
    }

    refreshInFlight = true

    while refreshPending {
      refreshPending = false

      let force = forcePending
      forcePending = false

      let cachedApplication = lastSnapshot.application
      let observationController = observationController
      let snapshot = await MainActor.run {
        copySnapshot(
          cachedApplication: cachedApplication,
          handleProcessIdentifier: { processIdentifier, detectionSource in
            observationController?.registerProcessIdentifier(
              processIdentifier,
              detectionSource: detectionSource
            )
          }
        )
      }
      await MainActor.run {
        observationController?.syncObservers(
          frontmostProcessIdentifier: snapshot.application?.processIdentifier
        )
      }

      if force || snapshot != lastSnapshot {
        await commitSnapshotAndEmit(snapshot, force: force)
      }
    }

    refreshInFlight = false
  }

  // In general, information about the currently focused application can be obtained through the following mechanisms:
  //
  // - Application switches can be detected via NSWorkspace.didActivateApplicationNotification.
  // - Window position and size changes can be detected via Accessibility notifications.
  //
  // However, some applications do not work with these mechanisms. Specifically, there are two categories:
  //
  // - Applications such as Spotlight, where NSWorkspace.didActivateApplicationNotification is not delivered.
  //   (Once the application has been detected at least once and AXObserverAddNotification has been registered for its process,
  //   subsequent detections can be made via kAXFocusedUIElementChangedNotification.)
  // - Applications such as Google Chrome and Electron-based apps that do not provide sufficient information through Accessibility.
  //   (window position and size changes cannot be obtained from notifications).
  //
  // For these applications, notification-based detection is not sufficient, so polling is required.
  // In particular, Spotlight-style application switches can be missed unless polling runs at a fairly high frequency.
  // For that reason, polling is performed every 500 ms.
  //
  // Because this polling needs to stay lightweight, requestRefresh is called only when necessary.
  // More specifically, requestRefresh is triggered only in the following cases:
  //
  // - When polling detects an application switch.
  // - When the current application's window position or size could not be obtained through the Accessibility API.
  //   (requestRefresh is called in order to fetch the latest window position and size.)
  private func refreshIfPollingNeedsSnapshot() async {
    let frontmostProcessIdentifier = await MainActor.run {
      copyFrontmostProcessIdentifier()
    }
    let applicationChanged =
      frontmostProcessIdentifier != lastSnapshot.application?.processIdentifier
    let needsGeometryPolling =
      lastSnapshot.focusedUIElement?.windowGeometrySource == .coreGraphics

    if applicationChanged || needsGeometryPolling {
      await requestRefresh(force: false)
    }
  }

  private func commitSnapshotAndEmit(_ snapshot: Snapshot, force: Bool) async {
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
                withOptionalCString(snapshot.focusedUIElement?.roleDescription) { roleDescription in
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
