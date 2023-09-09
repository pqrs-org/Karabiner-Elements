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
  var driverLoaded: Bool?
  var driverVersionMatched: Bool?
  var hidDeviceOpenPermitted: Bool?
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
      decoder.keyDecodingStrategy = .convertFromSnakeCase
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
      if state.value.driverLoaded == false {
        driverNotLoaded = true
      }
      if state.value.driverVersionMatched == false {
        driverVersionNotMatched = true
      }
      if state.value.hidDeviceOpenPermitted == false {
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

    print("needsDriverNotLoadedAlert \(needsDriverNotLoadedAlert)")
    print("needsDriverVersionNotMatchedAlert \(needsDriverVersionNotMatchedAlert)")
    print("needsInputMonitoringPermissionsAlert \(needsInputMonitoringPermissionsAlert)")

    //
    // Update alert window
    //

    updateAlertWindow(
      needsAlert: { return StateJsonMonitor.shared.needsDriverNotLoadedAlert },
      show: { AlertWindowsManager.shared.showDriverNotLoadedAlertWindow() },
      hide: { AlertWindowsManager.shared.hideDriverNotLoadedAlertWindow() })

    updateAlertWindow(
      needsAlert: { return StateJsonMonitor.shared.needsDriverVersionNotMatchedAlert },
      show: { AlertWindowsManager.shared.showDriverVersionNotMatchedAlertWindow() },
      hide: { AlertWindowsManager.shared.hideDriverVersionNotMatchedAlertWindow() })

    updateAlertWindow(
      needsAlert: { return StateJsonMonitor.shared.needsInputMonitoringPermissionsAlert },
      show: { AlertWindowsManager.shared.showInputMonitoringPermissionsAlertWindow() },
      hide: { AlertWindowsManager.shared.hideInputMonitoringPermissionsAlertWindow() })
  }

  private func updateAlertWindow(
    needsAlert: @escaping () -> Bool,
    show: @escaping () -> Void,
    hide: @escaping () -> Void
  ) {
    // Note:
    // Delay before displaying the alert to avoid the alert appearing momentarily.
    // (e.g, when karabiner_grabbedr is restarted)

    if needsAlert() {
      DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
        if needsAlert() {
          show()
        }
      }
    } else {
      hide()
    }
  }
}
