import AppKit
import Foundation
import SwiftUI

@objc
public class AlertWindowsManager: NSObject {
    @IBOutlet var parentWindow: NSWindow!
    var driverNotLoadedAlertWindow: NSWindow?
    var inputMonitoringPermissionsAlertWindow: NSWindow?

    @objc
    public func showDriverNotLoadedAlertWindow() {
        if driverNotLoadedAlertWindow == nil {
            driverNotLoadedAlertWindow = NSPanel(
                contentRect: .zero,
                styleMask: [
                    .titled,
                    .closable,
                    .fullSizeContentView,
                ],
                backing: .buffered,
                defer: false
            )
            driverNotLoadedAlertWindow!.title = DriverNotLoadedAlertView.title
            driverNotLoadedAlertWindow!.contentView = NSHostingView(rootView: DriverNotLoadedAlertView())
            driverNotLoadedAlertWindow!.centerToOtherWindow(parentWindow)
            parentWindow.addChildWindow(driverNotLoadedAlertWindow!, ordered: .above)
        }

        driverNotLoadedAlertWindow!.makeKeyAndOrderFront(nil)
    }

    @objc
    public func showInputMonitoringPermissionsAlertWindow() {
        if inputMonitoringPermissionsAlertWindow == nil {
            inputMonitoringPermissionsAlertWindow = NSPanel(
                contentRect: .zero,
                styleMask: [
                    .titled,
                    .closable,
                    .fullSizeContentView,
                ],
                backing: .buffered,
                defer: false
            )
            inputMonitoringPermissionsAlertWindow!.title = InputMonitoringPermissionsAlertView.title
            inputMonitoringPermissionsAlertWindow!.contentView = NSHostingView(rootView: InputMonitoringPermissionsAlertView())
            inputMonitoringPermissionsAlertWindow!.centerToOtherWindow(parentWindow)
            parentWindow.addChildWindow(inputMonitoringPermissionsAlertWindow!, ordered: .above)
        }

        inputMonitoringPermissionsAlertWindow!.makeKeyAndOrderFront(nil)
    }
}
