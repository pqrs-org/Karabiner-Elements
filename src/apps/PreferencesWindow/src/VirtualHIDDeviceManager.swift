import Foundation

class VirtualHIDDeviceManager {
    static let shared = VirtualHIDDeviceManager()

    func deactivateDriver() {
        run("deactivate")
    }

    func activateDriver() {
        run("activate")
    }

    private func run(_ argument: String) {
        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/Applications/.Karabiner-VirtualHIDDevice-Manager.app/Contents/MacOS/Karabiner-VirtualHIDDevice-Manager")
        task.arguments = [argument]
        try? task.run()
    }
}
