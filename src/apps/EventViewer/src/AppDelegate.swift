import Cocoa
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate, NSTabViewDelegate {
    @IBOutlet var window: NSWindow!
    @IBOutlet var eventQueue: EventQueue!
    @IBOutlet var keyResponder: KeyResponder!
    @IBOutlet var frontmostApplicationController: FrontmostApplicationController!
    @IBOutlet var variablesController: VariablesController!
    @IBOutlet var devicesController: DevicesController!

    var inputMonitoringAlertView: InputMonitoringAlertView?
    var inputMonitoringAlertWindow: NSWindow?

    public func applicationDidFinishLaunching(_: Notification) {
        libkrbn_initialize()

        setKeyResponder()
        setWindowProperty(self)
        eventQueue.setup()
        frontmostApplicationController.setup()
        variablesController.setup()
        devicesController.setup()

        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) { [weak self] in
            guard let self = self else { return }

            if !self.eventQueue.observed() {
                self.inputMonitoringAlertView = InputMonitoringAlertView()
                self.inputMonitoringAlertWindow = NSPanel(
                    contentRect: .zero,
                    styleMask: [
                        .titled,
                        .closable,
                        .fullSizeContentView,
                    ],
                    backing: .buffered,
                    defer: false
                )
                self.inputMonitoringAlertWindow!.title = "Input Monitoring Permissions Alert"
                self.inputMonitoringAlertWindow!.contentView = NSHostingView(rootView: self.inputMonitoringAlertView)
                self.inputMonitoringAlertWindow!.centerToOtherWindow(self.window)

                self.window.addChildWindow(self.inputMonitoringAlertWindow!, ordered: .above)
                self.inputMonitoringAlertWindow!.makeKeyAndOrderFront(nil)
            }
        }
    }

    // Note:
    // We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
    public func applicationWillTerminate(_: Notification) {
        libkrbn_terminate()
    }

    public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
        return true
    }

    public func windowWillClose(_: Notification) {
        inputMonitoringAlertWindow?.close()
    }

    public func tabView(_ tabView: NSTabView, didSelect _: NSTabViewItem?) {
        if tabView.identifier?.rawValue == "Main" {
            setKeyResponder()
        }
    }

    func setKeyResponder() {
        window.makeFirstResponder(keyResponder)
    }

    @IBAction func setWindowProperty(_: Any) {
        // ----------------------------------------
        if UserSettings.shared.forceStayTop {
            window.level = .floating
        } else {
            window.level = .normal
        }

        // ----------------------------------------
        if UserSettings.shared.showInAllSpaces {
            window.collectionBehavior.insert(.canJoinAllSpaces)
        } else {
            window.collectionBehavior.remove(.canJoinAllSpaces)
        }

        window.collectionBehavior.insert(.managed)
        window.collectionBehavior.remove(.moveToActiveSpace)
        window.collectionBehavior.remove(.transient)
    }
}
