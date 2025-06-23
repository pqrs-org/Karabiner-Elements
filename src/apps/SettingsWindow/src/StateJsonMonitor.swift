import Foundation

private func callback() {
  Task { @MainActor in
    StateJsonMonitor.shared.update()
  }
}

@MainActor
private struct State: Codable {
  var driverActivated: Bool?
  var driverVersionMismatched: Bool?
  var hidDeviceOpenPermitted: Bool?

  public mutating func update(_ s: State) {
    driverActivated = s.driverActivated ?? driverActivated
    driverVersionMismatched = s.driverVersionMismatched ?? driverVersionMismatched
    hidDeviceOpenPermitted = s.hidDeviceOpenPermitted ?? hidDeviceOpenPermitted
  }
}

@MainActor
public class StateJsonMonitor {
  static let shared = StateJsonMonitor()

  let grabberStateJsonFilePath = LibKrbn.grabberStateJsonFilePath()

  private var state = State()

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_file_monitors()

    libkrbn_register_file_updated_callback(
      grabberStateJsonFilePath.cString(using: .utf8),
      callback)
    libkrbn_enqueue_callback(callback)
  }

  public func stop() {
    libkrbn_unregister_file_updated_callback(
      grabberStateJsonFilePath.cString(using: .utf8),
      callback)

    // We don't call `libkrbn_disable_file_monitors` because the file monitors may be used elsewhere.
  }

  public func update() {
    if let jsonData = try? Data(contentsOf: URL(fileURLWithPath: grabberStateJsonFilePath)) {
      let decoder = JSONDecoder()
      decoder.keyDecodingStrategy = .convertFromSnakeCase
      if let s = try? decoder.decode(State.self, from: jsonData) {
        state.update(s)
      }
    }

    //
    // Update show alerts
    //

    if let v = state.driverActivated {
      print("driverActivated \(v)")
    }
    if let v = state.driverVersionMismatched {
      print("driverVersionMismatched \(v)")
    }
    if let v = state.hidDeviceOpenPermitted {
      print("hidDeviceOpenPermitted \(v)")
    }

    let contentViewStates = ContentViewStates.shared

    switch state.driverActivated {
    case true:
      contentViewStates.showDriverNotActivatedAlert = false
    case false:
      contentViewStates.showDriverNotActivatedAlert = true
    default:  // nil
      break
    }

    switch state.driverVersionMismatched {
    case true:
      contentViewStates.showDriverVersionMismatchedAlert = true
    case false:
      contentViewStates.showDriverVersionMismatchedAlert = false
    default:  // nil
      break
    }

    switch state.hidDeviceOpenPermitted {
    case true:
      contentViewStates.showInputMonitoringPermissionsAlert = false
    case false:
      contentViewStates.showInputMonitoringPermissionsAlert = true
    default:  // nil
      break
    }
  }
}
