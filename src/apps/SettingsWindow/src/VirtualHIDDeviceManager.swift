import Foundation

@MainActor
class VirtualHIDDeviceManager {
  static let shared = VirtualHIDDeviceManager()

  func deactivateDriver(completion: @escaping (Int32) -> Void) {
    run(
      argument: "deactivate",
      completion: completion)
  }

  func activateDriver(completion: @escaping (Int32) -> Void) {
    run(
      argument: "forceActivate",
      completion: completion)
  }

  private func run(argument: String, completion: @escaping (Int32) -> Void) {
    Task {
      let task = Process()
      task.executableURL = URL(
        fileURLWithPath:
          "/Applications/.Karabiner-VirtualHIDDevice-Manager.app/Contents/MacOS/Karabiner-VirtualHIDDevice-Manager"
      )
      task.arguments = [argument]

      try? task.run()

      task.waitUntilExit()
      let status = task.terminationStatus
      completion(status)
    }
  }
}
