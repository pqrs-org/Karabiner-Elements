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
            let view = DriverVersionNotMatchedAlertView(onCloseButtonPressed: { [weak self] in
                guard let self = self else { return }

                self.hideDriverVersionNotMatchedAlertWindow()
            })

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
            driverVersionNotMatchedAlertWindow!.contentView = NSHostingView(rootView: view)

            parentWindow.beginSheet(driverVersionNotMatchedAlertWindow!) { [weak self] _ in
                guard let self = self else { return }

                self.driverVersionNotMatchedAlertWindow = nil
            }
        }
    }

    @objc
    public func hideDriverVersionNotMatchedAlertWindow() {
        if driverVersionNotMatchedAlertWindow != nil {
            parentWindow.endSheet(driverVersionNotMatchedAlertWindow!)
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
