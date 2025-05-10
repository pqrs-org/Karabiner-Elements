import SwiftUI

@main
struct GamePadViewerAppApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  init() {
    libkrbn_initialize()

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }

    EventObserver.shared.start()
  }

  var body: some Scene {
    WindowGroup {
      ContentView()
    }
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}
