import Foundation

private func grabberStateJsonFileUpdatedCallback() {
  Task { @MainActor in
    StateJsonMonitor.shared.update(StateJsonMonitor.shared.grabberStateJsonFilePath)
  }
}

private func observerStateJsonFileUpdatedCallback() {
  Task { @MainActor in
    StateJsonMonitor.shared.update(StateJsonMonitor.shared.observerStateJsonFilePath)
  }
}

private struct State: Codable {
  var driverActivated: Bool?
  var driverVersionMismatched: Bool?
  var hidDeviceOpenPermitted: Bool?
}

public class StateJsonMonitor {
  static let shared = StateJsonMonitor()

  let grabberStateJsonFilePath = LibKrbn.grabberStateJsonFilePath()
  let observerStateJsonFilePath = LibKrbn.observerStateJsonFilePath()

  private var states: [String: State] = [:]

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_file_monitors()

    libkrbn_register_file_updated_callback(
      grabberStateJsonFilePath.cString(using: .utf8),
      grabberStateJsonFileUpdatedCallback)
    libkrbn_enqueue_callback(grabberStateJsonFileUpdatedCallback)

    libkrbn_register_file_updated_callback(
      observerStateJsonFilePath.cString(using: .utf8),
      observerStateJsonFileUpdatedCallback)
    libkrbn_enqueue_callback(observerStateJsonFileUpdatedCallback)
  }

  public func stop() {
    libkrbn_unregister_file_updated_callback(
      grabberStateJsonFilePath.cString(using: .utf8),
      grabberStateJsonFileUpdatedCallback)

    libkrbn_unregister_file_updated_callback(
      observerStateJsonFilePath.cString(using: .utf8),
      observerStateJsonFileUpdatedCallback)

    // We don't call `libkrbn_disable_file_monitors` because the file monitors may be used elsewhere.
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
