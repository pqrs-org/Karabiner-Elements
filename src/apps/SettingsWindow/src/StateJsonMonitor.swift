import Foundation

private func callback(
  _ filePath: UnsafePointer<Int8>?,
  _ context: UnsafeMutableRawPointer?
) {
  let obj: StateJsonMonitor! = unsafeBitCast(context, to: StateJsonMonitor.self)
  let path = String(cString: filePath!)

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    obj.update(path)
  }
}

private struct State: Codable {
  var driver_loaded: Bool?
  var driver_version_matched: Bool?
  var hid_device_open_permitted: Bool?
}

public class StateJsonMonitor {
  static let shared = StateJsonMonitor()

  private var states: [String: State] = [:]

  public var needsDriverNotLoadedAlert = false
  public var needsDriverVersionNotMatchedAlert = false
  public var needsInputMonitoringPermissionsAlert = false

  public func start() {
    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_observer_state_json_file_monitor(callback, obj)
    libkrbn_enable_grabber_state_json_file_monitor(callback, obj)
  }

  public func stop() {
    libkrbn_disable_observer_state_json_file_monitor()
    libkrbn_disable_grabber_state_json_file_monitor()
  }

  public func update(_ filePath: String) {
    if let jsonData = try? Data(contentsOf: URL(fileURLWithPath: filePath)) {
      let decoder = JSONDecoder()
      if let state = try? decoder.decode(State.self, from: jsonData) {
        states[filePath] = state
      }
    }

    //
    // Show alerts
    //

    var driverNotLoaded = false
    var driverVersionNotMatched = false
    var inputMonitoringNotPermitted = false
    for state in states {
      if state.value.driver_loaded == false {
        driverNotLoaded = true
      }
      if state.value.driver_version_matched == false {
        driverVersionNotMatched = true
      }
      if state.value.hid_device_open_permitted == false {
        inputMonitoringNotPermitted = true
      }
    }

    // print("driverNotLoaded \(driverNotLoaded)")
    // print("driverVersionNotMatched \(driverVersionNotMatched)")
    // print("inputMonitoringNotPermitted \(inputMonitoringNotPermitted)")

    //
    // - DriverNotLoadedAlertWindow
    // - DriverVersionNotMatchedAlertWindow
    //

    if needsDriverVersionNotMatchedAlert {
      // If DriverVersionNotMatchedAlertWindow is shown,
      // keep needsDriverNotLoadedAlert to prevent showing DriverNotLoadedAlertWindow after the driver is deactivated.

      if !driverVersionNotMatched {
        needsDriverVersionNotMatchedAlert = false
      }

    } else {
      if driverNotLoaded {
        needsDriverNotLoadedAlert = true
        needsDriverVersionNotMatchedAlert = false
      } else {
        needsDriverNotLoadedAlert = false

        if driverVersionNotMatched {
          needsDriverVersionNotMatchedAlert = true
        } else {
          needsDriverVersionNotMatchedAlert = false
        }
      }
    }

    //
    // - InputMonitoringPermissionsAlertWindow
    //

    if inputMonitoringNotPermitted {
      needsInputMonitoringPermissionsAlert = true
    } else {
      needsInputMonitoringPermissionsAlert = false
    }

    // print("needsDriverNotLoadedAlert \(needsDriverNotLoadedAlert)")
    // print("needsDriverVersionNotMatchedAlert \(needsDriverVersionNotMatchedAlert)")
    // print("needsInputMonitoringPermissionsAlert \(needsInputMonitoringPermissionsAlert)")

    //
    // Update alert window
    //

    updateAlertWindow(needsDriverNotLoadedAlert) {
      AlertWindowsManager.shared.updateDriverNotLoadedAlertWindow()
    }
    updateAlertWindow(needsDriverVersionNotMatchedAlert) {
      AlertWindowsManager.shared.updateDriverVersionNotMatchedAlertWindow()
    }
    updateAlertWindow(needsInputMonitoringPermissionsAlert) {
      AlertWindowsManager.shared.updateInputMonitoringPermissionsAlertWindow()
    }
  }

  private func updateAlertWindow(_ needsAlert: Bool, execute work: @escaping () -> Void) {
    // Delay before displaying the alert to avoid the alert appearing momentarily.
    // (e.g, when karabiner_grabbedr is restarted)

    DispatchQueue.main.asyncAfter(
      deadline: .now() + (needsAlert ? 3 : 0),
      execute: work)
  }
}
