import Cocoa
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    var window: NSWindow?
    var inputMonitoringAlertWindow: NSWindow?

    public func applicationDidFinishLaunching(_: Notification) {
        ProcessInfo.processInfo.enableSuddenTermination()

        libkrbn_initialize()

        window = NSWindow(
            contentRect: .zero,
            styleMask: [
                .titled,
                .closable,
                .miniaturizable,
                .resizable,
                .fullSizeContentView,
            ],
            backing: .buffered,
            defer: false
        )
        window!.contentView = NSHostingView(rootView: ContentView())
        window!.center()
        window!.makeKeyAndOrderFront(self)

        setWindowProperty()

        NotificationCenter.default.addObserver(forName: UserSettings.windowSettingChanged,
                                               object: nil,
                                               queue: .main) { [weak self] _ in
            guard let self = self else { return }

            self.setWindowProperty()
        }

        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) { [weak self] in
            guard let self = self else { return }

            if !EventHistory.shared.observed() {
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
                self.inputMonitoringAlertWindow!.centerToOtherWindow(self.window!)

                self.window!.addChildWindow(self.inputMonitoringAlertWindow!, ordered: .above)
                self.inputMonitoringAlertWindow!.makeKeyAndOrderFront(nil)
            }
        }
    }

    public func applicationWillTerminate(_: Notification) {
        libkrbn_terminate()
    }

    public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
        true
    }

    private func setWindowProperty() {
        if let window = self.window {
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
}

extension AppDelegate: NSWindowDelegate {
    public func windowWillClose(_: Notification) {
        NSApplication.shared.terminate(self)
    }
}
