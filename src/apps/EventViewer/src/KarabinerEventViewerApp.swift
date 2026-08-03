import Cocoa
import SwiftUI

@main
struct KarabinerEventViewerApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  @StateObject private var userSettings: UserSettings

  init() {
    _ = EVCoreServiceDaemonClient.shared
    _ = FrontmostApplicationHistory.shared
    _ = EventHistory.shared

    krbn_initialize(
      coreServiceConnectionChangedCallback,
      manipulatorEnvironmentReceivedCallback,
      connectedDevicesReceivedCallback,
      frontmostApplicationHistoryReceivedCallback,
      hidValueMonitorStoppedCallback,
      hidValueArrivedCallback)

    let userSettings = UserSettings()
    _userSettings = StateObject(wrappedValue: userSettings)

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }

    FrontmostApplicationHistory.shared.watch()

    NSEvent.addLocalMonitorForEvents(matching: .keyDown) { event -> NSEvent? in
      if event.modifierFlags.intersection(.deviceIndependentFlagsMask) == .command {
        if event.charactersIgnoringModifiers == "q" || event.charactersIgnoringModifiers == "w" {
          if userSettings.quitUsingKeyboardShortcut {
            NSApplication.shared.terminate(nil)
          }
          return nil
        }
      }
      return event
    }
  }

  var body: some Scene {
    Window(
      "Karabiner-EventViewer",
      id: "main",
      content: {
        ContentView()
          .environmentObject(userSettings)
      }
    )
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationWillTerminate(_: Notification) {
    krbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}
