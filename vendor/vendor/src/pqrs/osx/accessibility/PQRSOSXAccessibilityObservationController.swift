// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import ApplicationServices

private let observedAccessibilityNotifications: [CFString] = [
  kAXFocusedUIElementChangedNotification as CFString,
  kAXFocusedWindowChangedNotification as CFString,
  kAXMainWindowChangedNotification as CFString,
  kAXWindowMovedNotification as CFString,
  kAXWindowResizedNotification as CFString,
]

private let accessibilityNotificationRetryClock = ContinuousClock()
private let accessibilityNotificationRetryInterval = Duration.seconds(10)
// A title element whose notification removal fails transiently must remain
// retained so a later synchronization can retry the removal. If the focused
// window keeps changing while removals continue to fail, these stale elements
// and their notification registrations can accumulate. Recreate the observer
// at this limit to release them as a group and bound resource usage, while
// preserving the existing observer for ordinary transient failures below it.
private let maximumRetainedStaleTitleNotificationElements = 8

func staleTitleNotificationElementsRequireObserverRecreation(
  _ retainedElementCount: Int
) -> Bool {
  retainedElementCount >= maximumRetainedStaleTitleNotificationElements
}

enum AccessibilityNotificationAddDisposition: Equatable {
  // Record the notification as registered, including when it was already
  // registered before this attempt.
  case registered

  // Preserve the pending registration and try it again after the retry interval.
  case retry

  // Stop trying to register the notification because retrying is not useful.
  case stopTrying

  // The observer itself is invalid, so discard all of its registrations and
  // recreate it after the retry interval.
  case invalidateObserver
}

func accessibilityNotificationAddDisposition(
  _ error: AXError
) -> AccessibilityNotificationAddDisposition {
  switch error {
  case .success, .notificationAlreadyRegistered:
    return .registered

  case .cannotComplete, .failure, .apiDisabled:
    return .retry

  case .invalidUIElementObserver:
    return .invalidateObserver

  default:
    return .stopTrying
  }
}

enum AccessibilityNotificationRemoveDisposition: Equatable {
  // Preserve the tracked registration and try removing it again after the
  // retry interval.
  case retry

  // Stop tracking the registration because it has already been removed or
  // retrying the removal is not useful.
  case stopTracking

  // The observer itself is invalid, so discard all of its registrations and
  // recreate it after the retry interval.
  case invalidateObserver
}

// Converts an AXObserverRemoveNotification result into the follow-up action.
// Successful removals, missing registrations, and errors that cannot benefit
// from a retry all stop tracking the registration. Transient errors are retried,
// and an invalid observer is recreated.
func accessibilityNotificationRemoveDisposition(
  _ error: AXError
) -> AccessibilityNotificationRemoveDisposition {
  switch error {
  case .cannotComplete, .failure, .apiDisabled:
    return .retry

  case .invalidUIElementObserver:
    return .invalidateObserver

  default:
    return .stopTracking
  }
}

private struct AccessibilityObserverRegistration {
  let observer: AXObserver
  let applicationElement: AXUIElement
  var applicationNotificationsToRetry = observedAccessibilityNotifications
  var applicationNotificationRetryAfter: ContinuousClock.Instant?
  var requestedTitleNotificationElements: [AXUIElement] = []
  var titleNotificationElements: [AXUIElement] = []
  var titleNotificationElementsToRetry: [AXUIElement] = []
  var titleNotificationRetryAfter: ContinuousClock.Instant?
}

private func containsAXUIElement(_ elements: [AXUIElement], _ candidate: AXUIElement) -> Bool {
  elements.contains { CFEqual($0, candidate) }
}

private func containsAXUIElements(_ elements: [AXUIElement], _ candidates: [AXUIElement]) -> Bool {
  candidates.allSatisfy { containsAXUIElement(elements, $0) }
}

private func containsSameAXUIElements(_ lhs: [AXUIElement], _ rhs: [AXUIElement]) -> Bool {
  lhs.count == rhs.count
    && containsAXUIElements(lhs, rhs)
}

func titleNotificationsNeedSynchronization(
  requestedElements: [AXUIElement],
  registeredElements: [AXUIElement],
  desiredElements: [AXUIElement]
) -> Bool {
  let requestedElementsAreUnchanged = containsSameAXUIElements(
    requestedElements,
    desiredElements
  )
  let registeredElementsContainNoStaleElements = containsAXUIElements(
    desiredElements,
    registeredElements
  )

  return !requestedElementsAreUnchanged || !registeredElementsContainNoStaleElements
}

private let accessibilityObserverCallback: AXObserverCallback = { _, _, _, refcon in
  guard let refcon else {
    return
  }

  let callbackGeneration = Int(bitPattern: refcon)
  Task { @MainActor in
    PQRSOSXAccessibility.Monitor.shared.scheduleAccessibilityNotificationRefresh(
      callbackGeneration: callbackGeneration
    )
  }
}

extension PQRSOSXAccessibility {
  @MainActor
  final class ObservationController {
    private var activationObserver: NSObjectProtocol?
    private var terminationObserver: NSObjectProtocol?
    // PIDs that have been observed through NSWorkspace activation notifications.
    private var workspaceKnownPIDs: Set<pid_t> = []
    // PIDs discovered outside NSWorkspace that still need AXObserver-based tracking.
    private var observerManagedPIDs: Set<pid_t> = []
    private var observerRegistrationsByPID: [pid_t: AccessibilityObserverRegistration] = [:]
    private var observerAttachmentRetryAfterByPID: [pid_t: ContinuousClock.Instant] = [:]
    // The current frontmost PID used to keep frontmost-app observation attached.
    private var frontmostProcessIdentifier: pid_t?
    private var frontmostTitleNotificationElements: [AXUIElement] = []
    private var callbackGeneration = 0

    func start(callbackGeneration: Int) {
      guard activationObserver == nil, terminationObserver == nil else {
        return
      }

      self.callbackGeneration = callbackGeneration

      activationObserver = NSWorkspace.shared.notificationCenter.addObserver(
        forName: NSWorkspace.didActivateApplicationNotification,
        object: nil,
        queue: nil
      ) { [weak self] _ in
        guard let self else {
          return
        }

        Task { @MainActor in
          guard self.isCurrentCallbackGeneration(callbackGeneration) else {
            return
          }

          self.requestRefresh(callbackGeneration: callbackGeneration)
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
          guard self.isCurrentCallbackGeneration(callbackGeneration) else {
            return
          }

          let processIdentifier =
            (notification.userInfo?[NSWorkspace.applicationUserInfoKey] as? NSRunningApplication)?
            .processIdentifier

          self.pruneProcessIdentifier(processIdentifier)
          self.requestRefresh(callbackGeneration: callbackGeneration)
        }
      }

      requestRefresh(callbackGeneration: callbackGeneration)
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

      for processIdentifier in Array(observerRegistrationsByPID.keys) {
        detachObserver(processIdentifier: processIdentifier)
      }

      workspaceKnownPIDs.removeAll()
      observerManagedPIDs.removeAll()
      observerRegistrationsByPID.removeAll()
      observerAttachmentRetryAfterByPID.removeAll()
      frontmostProcessIdentifier = nil
      frontmostTitleNotificationElements.removeAll()
      callbackGeneration = 0
    }

    func registerProcessIdentifier(_ processIdentifier: pid_t?, detectionSource: DetectionSource) {
      guard let processIdentifier, processIdentifier != 0 else {
        return
      }

      switch detectionSource {
      case .workspace:
        workspaceKnownPIDs.insert(processIdentifier)
        observerManagedPIDs.remove(processIdentifier)

      case .axObserver:
        guard !workspaceKnownPIDs.contains(processIdentifier) else {
          return
        }

        observerManagedPIDs.insert(processIdentifier)

      case .none:
        break
      }
    }

    func pruneProcessIdentifier(_ processIdentifier: pid_t?) {
      guard let processIdentifier, processIdentifier != 0 else {
        return
      }

      workspaceKnownPIDs.remove(processIdentifier)
      observerManagedPIDs.remove(processIdentifier)

      if frontmostProcessIdentifier == processIdentifier {
        frontmostProcessIdentifier = nil
        frontmostTitleNotificationElements.removeAll()
      }

      detachObserver(processIdentifier: processIdentifier)
    }

    func pruneStaleProcessIdentifiers() {
      let knownProcessIdentifiers =
        workspaceKnownPIDs
        .union(observerManagedPIDs)
        .union(observerRegistrationsByPID.keys)
        .union(observerAttachmentRetryAfterByPID.keys)

      for processIdentifier in knownProcessIdentifiers
      where NSRunningApplication(processIdentifier: processIdentifier) == nil {
        pruneProcessIdentifier(processIdentifier)
      }
    }

    private func requestRefresh(callbackGeneration: Int) {
      Task { @MainActor in
        PQRSOSXAccessibility.Monitor.shared.requestRefresh(
          force: false,
          callbackGeneration: callbackGeneration
        )
      }
    }

    private func isCurrentCallbackGeneration(_ callbackGeneration: Int) -> Bool {
      self.callbackGeneration == callbackGeneration
    }

    func syncObservers(
      frontmostProcessIdentifier: pid_t?,
      titleNotificationElements: [AXUIElement]
    ) {
      self.frontmostProcessIdentifier = frontmostProcessIdentifier
      frontmostTitleNotificationElements = titleNotificationElements

      var targetPIDs = observerManagedPIDs.subtracting(workspaceKnownPIDs)

      if let frontmostProcessIdentifier, frontmostProcessIdentifier != 0 {
        targetPIDs.insert(frontmostProcessIdentifier)
      }

      let observerPIDs = Set(observerRegistrationsByPID.keys)
        .union(observerAttachmentRetryAfterByPID.keys)
      let stalePIDs = observerPIDs.subtracting(targetPIDs)
      for processIdentifier in stalePIDs {
        detachObserver(processIdentifier: processIdentifier)
      }

      for processIdentifier in targetPIDs {
        attachObserver(processIdentifier: processIdentifier)
      }

      for processIdentifier in Array(observerRegistrationsByPID.keys) {
        syncApplicationNotifications(processIdentifier: processIdentifier)
        syncTitleNotifications(
          processIdentifier: processIdentifier,
          elements: processIdentifier == frontmostProcessIdentifier
            ? titleNotificationElements : []
        )
      }
    }

    func windowTitleNeedsRefresh(currentWindowTitle: String?) -> Bool {
      guard !frontmostTitleNotificationElements.isEmpty else {
        return false
      }

      if let frontmostProcessIdentifier,
        let registration = observerRegistrationsByPID[frontmostProcessIdentifier],
        containsAXUIElements(
          registration.titleNotificationElements,
          registration.requestedTitleNotificationElements
        )
      {
        return false
      }

      var didReadTitle = false
      var latestWindowTitle: String?
      for element in frontmostTitleNotificationElements {
        var value: CFTypeRef?
        let error = AXUIElementCopyAttributeValue(
          element,
          kAXTitleAttribute as CFString,
          &value
        )
        guard error == .success else {
          continue
        }

        didReadTitle = true
        if let title = value as? String, !title.isEmpty {
          latestWindowTitle = title
          break
        }
      }

      // A transient AX failure is not evidence that the title was cleared.
      guard didReadTitle else {
        return false
      }

      return latestWindowTitle != currentWindowTitle
    }

    private func attachObserver(processIdentifier: pid_t) {
      guard processIdentifier != 0 else {
        return
      }

      guard observerRegistrationsByPID[processIdentifier] == nil else {
        return
      }

      if let retryAfter = observerAttachmentRetryAfterByPID[processIdentifier],
        retryAfter > accessibilityNotificationRetryClock.now
      {
        return
      }

      var observer: AXObserver?
      let error = AXObserverCreate(processIdentifier, accessibilityObserverCallback, &observer)
      guard error == .success, let observer else {
        scheduleObserverAttachmentRetry(processIdentifier: processIdentifier)
        return
      }

      observerAttachmentRetryAfterByPID.removeValue(forKey: processIdentifier)

      let applicationElement = AXUIElementCreateApplication(processIdentifier)
      var registration = AccessibilityObserverRegistration(
        observer: observer,
        applicationElement: applicationElement
      )
      guard attemptApplicationNotificationRegistration(&registration) else {
        scheduleObserverAttachmentRetry(processIdentifier: processIdentifier)
        return
      }

      CFRunLoopAddSource(
        CFRunLoopGetMain(),
        AXObserverGetRunLoopSource(observer),
        .commonModes
      )

      observerRegistrationsByPID[processIdentifier] = registration
    }

    private func syncApplicationNotifications(processIdentifier: pid_t) {
      guard var registration = observerRegistrationsByPID[processIdentifier] else {
        return
      }

      guard !registration.applicationNotificationsToRetry.isEmpty else {
        return
      }

      if let retryAfter = registration.applicationNotificationRetryAfter,
        retryAfter > accessibilityNotificationRetryClock.now
      {
        return
      }

      guard attemptApplicationNotificationRegistration(&registration) else {
        scheduleObserverAttachmentRetry(processIdentifier: processIdentifier)
        return
      }

      observerRegistrationsByPID[processIdentifier] = registration
    }

    // Returns false when the observer itself is invalid and must be recreated.
    private func attemptApplicationNotificationRegistration(
      _ registration: inout AccessibilityObserverRegistration
    ) -> Bool {
      var notificationsToRetry: [CFString] = []

      for notification in registration.applicationNotificationsToRetry {
        let error = AXObserverAddNotification(
          registration.observer,
          registration.applicationElement,
          notification,
          UnsafeMutableRawPointer(bitPattern: callbackGeneration)
        )

        switch accessibilityNotificationAddDisposition(error) {
        case .registered, .stopTrying:
          break
        case .retry:
          notificationsToRetry.append(notification)
        case .invalidateObserver:
          return false
        }
      }

      registration.applicationNotificationsToRetry = notificationsToRetry
      registration.applicationNotificationRetryAfter =
        notificationsToRetry.isEmpty
        ? nil
        : notificationRetryAfter()
      return true
    }

    private func syncTitleNotifications(processIdentifier: pid_t, elements: [AXUIElement]) {
      guard var registration = observerRegistrationsByPID[processIdentifier] else {
        return
      }

      registration.titleNotificationElementsToRetry =
        registration.titleNotificationElementsToRetry.filter {
          containsAXUIElement(elements, $0)
        }

      let requestedElementsChanged = !containsSameAXUIElements(
        registration.requestedTitleNotificationElements,
        elements
      )
      guard
        titleNotificationsNeedSynchronization(
          requestedElements: registration.requestedTitleNotificationElements,
          registeredElements: registration.titleNotificationElements,
          desiredElements: elements
        )
          || !registration.titleNotificationElementsToRetry.isEmpty
      else {
        return
      }

      if !requestedElementsChanged,
        let retryAfter = registration.titleNotificationRetryAfter,
        retryAfter > accessibilityNotificationRetryClock.now
      {
        return
      }

      var registeredElements: [AXUIElement] = []
      var retryableElements = registration.titleNotificationElementsToRetry
      var needsRetry = false
      var retainedStaleElementCount = 0
      for element in registration.titleNotificationElements {
        if containsAXUIElement(elements, element) {
          registeredElements.append(element)
          continue
        }

        let error = AXObserverRemoveNotification(
          registration.observer,
          element,
          kAXTitleChangedNotification as CFString
        )
        switch accessibilityNotificationRemoveDisposition(error) {
        case .stopTracking:
          break
        case .retry:
          // Keep tracking the element so a later snapshot can retry removal.
          registeredElements.append(element)
          needsRetry = true
          retainedStaleElementCount += 1
        case .invalidateObserver:
          scheduleObserverAttachmentRetry(processIdentifier: processIdentifier)
          return
        }
      }

      if staleTitleNotificationElementsRequireObserverRecreation(retainedStaleElementCount) {
        scheduleObserverAttachmentRetry(processIdentifier: processIdentifier)
        return
      }

      let elementsToRegister =
        requestedElementsChanged
        ? elements
        : retryableElements
      retryableElements.removeAll()

      for element in elementsToRegister
      where containsAXUIElement(elements, element)
        && !containsAXUIElement(registeredElements, element)
      {
        let error = AXObserverAddNotification(
          registration.observer,
          element,
          kAXTitleChangedNotification as CFString,
          UnsafeMutableRawPointer(bitPattern: callbackGeneration)
        )
        switch accessibilityNotificationAddDisposition(error) {
        case .registered:
          registeredElements.append(element)
        case .retry:
          retryableElements.append(element)
          needsRetry = true
        case .stopTrying:
          break
        case .invalidateObserver:
          scheduleObserverAttachmentRetry(processIdentifier: processIdentifier)
          return
        }
      }

      registration.requestedTitleNotificationElements = elements
      registration.titleNotificationElements = registeredElements
      registration.titleNotificationElementsToRetry = retryableElements
      registration.titleNotificationRetryAfter = needsRetry ? notificationRetryAfter() : nil
      observerRegistrationsByPID[processIdentifier] = registration
    }

    private func notificationRetryAfter() -> ContinuousClock.Instant {
      accessibilityNotificationRetryClock.now.advanced(
        by: accessibilityNotificationRetryInterval
      )
    }

    private func scheduleObserverAttachmentRetry(processIdentifier: pid_t) {
      detachObserver(processIdentifier: processIdentifier)
      observerAttachmentRetryAfterByPID[processIdentifier] = notificationRetryAfter()
    }

    private func detachObserver(processIdentifier: pid_t) {
      if let registration = observerRegistrationsByPID.removeValue(forKey: processIdentifier) {
        CFRunLoopRemoveSource(
          CFRunLoopGetMain(),
          AXObserverGetRunLoopSource(registration.observer),
          .commonModes
        )
      }
      observerAttachmentRetryAfterByPID.removeValue(forKey: processIdentifier)
    }
  }
}
