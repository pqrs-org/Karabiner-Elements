import Foundation

private func callback(_ filePath: UnsafePointer<Int8>?,
                      _ context: UnsafeMutableRawPointer?)
{
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

@objc
public class StateJsonMonitor: NSObject {
    @IBOutlet private var alertWindowsManager: AlertWindowsManager!
    private var states: [String: State] = [:]
    private var driverVersionNotMatchedAlertViewShown = false

    @objc
    public func start() {
        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_observer_state_json_file_monitor(callback, obj)
        libkrbn_enable_grabber_state_json_file_monitor(callback, obj)
    }

    @objc
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

        //
        // - DriverNotLoadedAlertWindow
        // - DriverVersionNotMatchedAlertWindow
        //

        if driverVersionNotMatchedAlertViewShown {
            // If DriverVersionNotMatchedAlertWindow is shown,
            // do nothing here to prevent showing DriverNotLoadedAlertWindow after the driver is deactivated.
        } else {
            if driverNotLoaded {
                alertWindowsManager.showDriverNotLoadedAlertWindow()
            } else {
                alertWindowsManager.hideDriverNotLoadedAlertWindow()

                if driverVersionNotMatched {
                    alertWindowsManager.showDriverVersionNotMatchedAlertWindow()
                    driverVersionNotMatchedAlertViewShown = true
                } else {
                    alertWindowsManager.hideDriverVersionNotMatchedAlertWindow()
                }
            }
        }

        //
        // - InputMonitoringPermissionsAlertWindow
        //

        if inputMonitoringNotPermitted {
            alertWindowsManager.showInputMonitoringPermissionsAlertWindow()
        } else {
            alertWindowsManager.hideInputMonitoringPermissionsAlertWindow()
        }
    }
}
