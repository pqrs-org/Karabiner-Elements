import Foundation

private func callback() {
  Task { @MainActor in
    StateJsonMonitor.shared.update()
  }
}

@MainActor
private struct State: Codable {
  var virtualHidDeviceServiceClientConnected: Bool?
  var driverActivated: Bool?
  var driverConnected: Bool?
  var driverVersionMismatched: Bool?
  var hidDeviceOpenPermitted: Bool?

  public mutating func update(_ s: State) {
    virtualHidDeviceServiceClientConnected =
      s.virtualHidDeviceServiceClientConnected ?? virtualHidDeviceServiceClientConnected
    driverActivated = s.driverActivated ?? driverActivated
    driverConnected = s.driverConnected ?? driverConnected
    driverVersionMismatched = s.driverVersionMismatched ?? driverVersionMismatched
    hidDeviceOpenPermitted = s.hidDeviceOpenPermitted ?? hidDeviceOpenPermitted
  }
}

@MainActor
public class StateJsonMonitor {
  static let shared = StateJsonMonitor()

  let coreServiceStateJsonFilePath = LibKrbn.coreServiceStateJsonFilePath()

  private var state = State()

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_file_monitors()

    if let cString = coreServiceStateJsonFilePath.cString(using: .utf8) {
      libkrbn_register_file_updated_callback(cString, callback)
      libkrbn_enqueue_callback(callback)
    }
  }

  public func stop() {
    if let cString = coreServiceStateJsonFilePath.cString(using: .utf8) {
      libkrbn_unregister_file_updated_callback(cString, callback)
    }

    // We don't call `libkrbn_disable_file_monitors` because the file monitors may be used elsewhere.
  }

  public func update() {
    if let jsonData = try? Data(contentsOf: URL(fileURLWithPath: coreServiceStateJsonFilePath)) {
      let decoder = JSONDecoder()
      decoder.keyDecodingStrategy = .convertFromSnakeCase
      if let s = try? decoder.decode(State.self, from: jsonData) {
        state.update(s)
      }
    }

    //
    // Update show alerts
    //

    if let v = state.virtualHidDeviceServiceClientConnected {
      print("virtualHidDeviceServiceClientConnected \(v)")
    }
    if let v = state.driverActivated {
      print("driverActivated \(v)")
    }
    if let v = state.driverConnected {
      print("driverConnected \(v)")
    }
    if let v = state.driverVersionMismatched {
      print("driverVersionMismatched \(v)")
    }
    if let v = state.hidDeviceOpenPermitted {
      print("hidDeviceOpenPermitted \(v)")
    }

    let contentViewStates = ContentViewStates.shared

    switch state.virtualHidDeviceServiceClientConnected {
    case true:
      contentViewStates.showVirtualHidDeviceServiceClientNotConnectedAlert = false
    case false:
      contentViewStates.showVirtualHidDeviceServiceClientNotConnectedAlert = true
    default:  // nil
      break
    }

    switch state.driverActivated {
    case true:
      contentViewStates.showDriverNotActivatedAlert = false
    case false:
      contentViewStates.showDriverNotActivatedAlert = true
    default:  // nil
      break
    }

    switch state.driverConnected {
    case true:
      contentViewStates.showDriverNotConnectedAlert = false
    case false:
      contentViewStates.showDriverNotConnectedAlert = true
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
