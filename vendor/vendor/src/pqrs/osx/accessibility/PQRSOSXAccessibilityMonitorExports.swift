// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import Foundation

@_cdecl("pqrs_osx_accessibility_monitor_set_callback")
func PQRSOSXAccessibilityMonitorSetCallback(
  _ callback: @escaping PQRSOSXAccessibilityMonitorCallback
) {
  Task {
    await PQRSOSXAccessibilityMonitor.shared.setCallback(callback)
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_unset_callback")
func PQRSOSXAccessibilityMonitorUnsetCallback() {
  Task {
    await PQRSOSXAccessibilityMonitor.shared.unsetCallback()
  }
}

@_cdecl("pqrs_osx_accessibility_monitor_async_trigger")
func PQRSOSXAccessibilityMonitorAsyncTrigger() {
  Task {
    await PQRSOSXAccessibilityMonitor.shared.asyncTrigger()
  }
}
