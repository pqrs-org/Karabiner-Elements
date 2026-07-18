// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import Foundation

@inline(__always)
private func syncMainActorCall(_ operation: @MainActor () -> Void) {
  if Thread.isMainThread {
    MainActor.assumeIsolated {
      operation()
    }
  } else {
    DispatchQueue.main.sync {
      MainActor.assumeIsolated {
        operation()
      }
    }
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_set_callback")
func PQRSOSXAccessibilityMonitorSetCallback(
  _ callback: @escaping PQRSOSXAccessibility.MonitorCallback
) {
  syncMainActorCall {
    PQRSOSXAccessibility.Monitor.shared.setCallback(callback)
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_unset_callback")
func PQRSOSXAccessibilityMonitorUnsetCallback() {
  syncMainActorCall {
    PQRSOSXAccessibility.Monitor.shared.unsetCallback()
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_trigger")
func PQRSOSXAccessibilityMonitorTrigger() {
  syncMainActorCall {
    PQRSOSXAccessibility.Monitor.shared.trigger()
  }
}
