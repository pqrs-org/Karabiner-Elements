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
  var accessibilityProcessTrusted: Bool?

  public mutating func update(_ s: State) {
    virtualHidDeviceServiceClientConnected =
      s.virtualHidDeviceServiceClientConnected ?? virtualHidDeviceServiceClientConnected
    driverActivated = s.driverActivated ?? driverActivated
    driverConnected = s.driverConnected ?? driverConnected
    driverVersionMismatched = s.driverVersionMismatched ?? driverVersionMismatched
    hidDeviceOpenPermitted = s.hidDeviceOpenPermitted ?? hidDeviceOpenPermitted
    accessibilityProcessTrusted = s.accessibilityProcessTrusted ?? accessibilityProcessTrusted
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
    if let v = state.accessibilityProcessTrusted {
      print("accessibilityProcessTrusted \(v)")
    }

  }
}
