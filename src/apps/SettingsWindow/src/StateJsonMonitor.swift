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
  var driverActivated: Bool?
  var driverVersionMismatched: Bool?
  var hidDeviceOpenPermitted: Bool?
}

public class StateJsonMonitor {
  static let shared = StateJsonMonitor()

  private var states: [String: State] = [:]

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

    var driverNotActivated = false
    var driverVersionMismatched = false
    var inputMonitoringNotPermitted = false
    for state in states {
      if state.value.driverActivated == false {
        driverNotActivated = true
      }
      if state.value.driverVersionMismatched == true {
        driverVersionMismatched = true
      }
      if state.value.hidDeviceOpenPermitted == false {
        inputMonitoringNotPermitted = true
      }
    }

    // print("driverNotActivated \(driverNotActivated)")
    // print("driverVersionMismatched \(driverVersionMismatched)")
    // print("inputMonitoringNotPermitted \(inputMonitoringNotPermitted)")

    let contentViewStates = ContentViewStates.shared

    //
    // - DriverNotActivatedAlert
    // - DriverVersionMismatchedAlert
    //

    if contentViewStates.showDriverVersionMismatchedAlert {
      // If diverVersionMismatchedAlert is shown,
      // keep needsDriverNotActivatedAlert to prevent showing DriverNotActivatedAlert after the driver is deactivated.

      if !driverVersionMismatched {
        contentViewStates.showDriverVersionMismatchedAlert = false
      }

    } else {
      if driverNotActivated {
        contentViewStates.showDriverNotActivatedAlert = true
        contentViewStates.showDriverVersionMismatchedAlert = false
      } else {
        contentViewStates.showDriverNotActivatedAlert = false

        if driverVersionMismatched {
          contentViewStates.showDriverVersionMismatchedAlert = true
        } else {
          contentViewStates.showDriverVersionMismatchedAlert = false
        }
      }
    }

    //
    // - InputMonitoringPermissionsAlert
    //

    if inputMonitoringNotPermitted {
      contentViewStates.showInputMonitoringPermissionsAlert = true
    } else {
      contentViewStates.showInputMonitoringPermissionsAlert = false
    }
  }
}
