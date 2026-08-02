import SwiftUI

private func hidValueArrivedCallback(
  _ usagePage: Int32,
  _ usage: Int32,
  _ logicalMax: Int64,
  _ logicalMin: Int64,
  _ integerValue: Int64
) {
  Task { @MainActor in
    EventObserver.shared.update(usagePage, usage, logicalMax, logicalMin, integerValue)
  }
}

@main
struct GamePadViewerAppApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  init() {
    game_pad_viewer_initialize(hidValueArrivedCallback)

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }
  }

  var body: some Scene {
    WindowGroup {
      ContentView()
    }
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationWillTerminate(_: Notification) {
    game_pad_viewer_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}
