import AppKit
import Foundation
import SwiftUI

@objc
public class AlertWindowsManager: NSObject {
    @IBOutlet var parentWindow: NSWindow!
    var driverNotLoadedAlertWindow: NSWindow?

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
            driverNotLoadedAlertWindow!.title = "Driver Alert"
            driverNotLoadedAlertWindow!.contentView = NSHostingView(rootView: DriverNotLoadedAlertView())
            driverNotLoadedAlertWindow!.centerToOtherWindow(parentWindow)
            parentWindow.addChildWindow(driverNotLoadedAlertWindow!, ordered: .above)
        }

        driverNotLoadedAlertWindow!.makeKeyAndOrderFront(nil)
    }
}
