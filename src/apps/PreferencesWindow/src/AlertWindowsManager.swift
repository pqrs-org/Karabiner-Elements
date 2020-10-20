import AppKit
import Foundation
import SwiftUI

@objc
public class AlertWindowsManager: NSObject {
    @IBOutlet var parentWindow: NSWindow!
    var driverNotLoadedAlertWindow: NSWindow?
    var driverVersionNotMatchedAlertWindow: NSWindow?
    var inputMonitoringPermissionsAlertWindow: NSWindow?

    //
    // driverNotLoadedAlertWindow
    //

    @objc
    public func showDriverNotLoadedAlertWindow() {
        if driverNotLoadedAlertWindow == nil {
            print("new driverNotLoadedAlertWindow")
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
    public func hideDriverNotLoadedAlertWindow() {
        if driverNotLoadedAlertWindow != nil {
            parentWindow.removeChildWindow(driverNotLoadedAlertWindow!)
            driverNotLoadedAlertWindow!.close()
            driverNotLoadedAlertWindow = nil
        }
    }

    //
    // driverVersionNotMatchedAlertWindow
    //

    @objc
    public func showDriverVersionNotMatchedAlertWindow() {
        if driverVersionNotMatchedAlertWindow == nil {
            driverVersionNotMatchedAlertWindow = NSPanel(
                contentRect: .zero,
                styleMask: [
                    .titled,
                    .closable,
                    .fullSizeContentView,
                ],
                backing: .buffered,
                defer: false
            )
            driverVersionNotMatchedAlertWindow!.title = DriverVersionNotMatchedAlertView.title
            driverVersionNotMatchedAlertWindow!.contentView = NSHostingView(rootView: DriverVersionNotMatchedAlertView())
            driverVersionNotMatchedAlertWindow!.centerToOtherWindow(parentWindow)
            parentWindow.addChildWindow(driverVersionNotMatchedAlertWindow!, ordered: .above)
        }

        driverVersionNotMatchedAlertWindow!.makeKeyAndOrderFront(nil)
    }

    @objc
    public func hideDriverVersionNotMatchedAlertWindow() {
        if driverVersionNotMatchedAlertWindow != nil {
            parentWindow.removeChildWindow(driverVersionNotMatchedAlertWindow!)
            driverVersionNotMatchedAlertWindow!.close()
            driverVersionNotMatchedAlertWindow = nil
        }
    }

    //
    // inputMonitoringPermissionsAlertWindow
    //

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

    @objc
    public func hideInputMonitoringPermissionsAlertWindow() {
        if inputMonitoringPermissionsAlertWindow != nil {
            parentWindow.removeChildWindow(inputMonitoringPermissionsAlertWindow!)
            inputMonitoringPermissionsAlertWindow!.close()
            inputMonitoringPermissionsAlertWindow = nil
        }
    }
}
