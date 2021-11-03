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
            let view = DriverNotLoadedAlertView(parentWindow: parentWindow,
                                                onCloseButtonPressed: { [weak self] in
                                                    guard let self = self else { return }

                                                    self.hideDriverNotLoadedAlertWindow()
                                                })

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
            driverNotLoadedAlertWindow!.contentView = NSHostingView(rootView: view)

            parentWindow.beginSheet(driverNotLoadedAlertWindow!) { [weak self] _ in
                guard let self = self else { return }

                self.driverNotLoadedAlertWindow = nil
            }
        }
    }

    @objc
    public func hideDriverNotLoadedAlertWindow() {
        if driverNotLoadedAlertWindow != nil {
            parentWindow.endSheet(driverNotLoadedAlertWindow!)
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
            driverVersionNotMatchedAlertWindow!.hidesOnDeactivate = false
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
            let view = InputMonitoringPermissionsAlertView(parentWindow: parentWindow,
                                                           onCloseButtonPressed: { [weak self] in
                                                               guard let self = self else { return }

                                                               self.hideInputMonitoringPermissionsAlertWindow()
                                                           })

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
            inputMonitoringPermissionsAlertWindow!.contentView = NSHostingView(rootView: view)

            parentWindow.beginSheet(inputMonitoringPermissionsAlertWindow!) { [weak self] _ in
                guard let self = self else { return }

                self.inputMonitoringPermissionsAlertWindow = nil
            }
        }
    }

    @objc
    public func hideInputMonitoringPermissionsAlertWindow() {
        if inputMonitoringPermissionsAlertWindow != nil {
            parentWindow.endSheet(inputMonitoringPermissionsAlertWindow!)
        }
    }
}
