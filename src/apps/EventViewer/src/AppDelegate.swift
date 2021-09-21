import Cocoa
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    @IBOutlet var window: NSWindow!
    @IBOutlet var variablesController: VariablesController!
    @IBOutlet var devicesController: DevicesController!

    var inputMonitoringAlertWindow: NSWindow?

    private var newWindow: NSWindow?

    public func applicationDidFinishLaunching(_: Notification) {
        ProcessInfo.processInfo.enableSuddenTermination()

        libkrbn_initialize()

        setWindowProperty(self)
        variablesController.setup()
        devicesController.setup()

        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) { [weak self] in
            guard let self = self else { return }

            if !EventQueue.shared.observed() {
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
                self.inputMonitoringAlertWindow!.hidesOnDeactivate = false
                self.inputMonitoringAlertWindow!.title = "Input Monitoring Permissions Alert"
                self.inputMonitoringAlertWindow!.contentView = NSHostingView(rootView: InputMonitoringAlertView())
                self.inputMonitoringAlertWindow!.centerToOtherWindow(self.window)

                self.window.addChildWindow(self.inputMonitoringAlertWindow!, ordered: .above)
                self.inputMonitoringAlertWindow!.makeKeyAndOrderFront(nil)
            }
        }

        newWindow = NSWindow(
            contentRect: NSMakeRect(0, 0, 600, 500),
            styleMask: [
                .titled,
                .closable,
                .miniaturizable,
                .fullSizeContentView,
            ],
            backing: .buffered,
            defer: false
        )
        newWindow!.contentView = NSHostingView(rootView: ContentView())
        newWindow!.center()
        newWindow!.makeKeyAndOrderFront(self)
    }

    public func applicationWillTerminate(_: Notification) {
        libkrbn_terminate()
    }

    public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
        true
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

extension AppDelegate: NSWindowDelegate {
    public func windowWillClose(_: Notification) {
        NSApplication.shared.terminate(self)
    }
}
