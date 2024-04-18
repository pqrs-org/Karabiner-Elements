import Cocoa
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  var window: NSWindow?

  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }

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
    window!.title = "Karabiner-GamePadViewer"
    window!.contentView = NSHostingView(rootView: ContentView())
    window!.center()
    window!.makeKeyAndOrderFront(self)

    EventObserver.shared.start()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}

extension AppDelegate: NSWindowDelegate {
  public func windowWillClose(_: Notification) {
    NSApplication.shared.terminate(self)
  }
}
