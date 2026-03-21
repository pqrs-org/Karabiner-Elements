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

  func handleAccessibilityNotification(_ notification: CFString, processIdentifier: pid_t?) async {
    let observationController = observationController
    await MainActor.run {
      observationController?.handleAccessibilityNotification(
        notification,
        processIdentifier: processIdentifier
      )
    }
  }

  private func listenLoop() async {
    while !Task.isCancelled {
      do {
        try await Task.sleep(for: fallbackPollingInterval)
      } catch {
        break
      }

      await requestRefresh(force: false)
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
      let snapshot = await MainActor.run {
        copySnapshot(cachedApplication: cachedApplication)
      }

      let observationController = observationController
      await MainActor.run {
        observationController?.registerObserverManagedProcessIdentifier(
          snapshot.application?.processIdentifier
        )
        observationController?.syncObservers(
          workspaceFrontmostProcessIdentifier: snapshot.application?.processIdentifier
        )
      }

      if force || snapshot != lastSnapshot {
        await commitSnapshotAndEmit(snapshot, force: force)
      }
    }

    refreshInFlight = false
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
