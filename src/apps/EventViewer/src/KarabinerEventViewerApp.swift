import Cocoa
import SwiftUI

@main
struct KarabinerEventViewerApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  @StateObject private var userSettings: UserSettings

  init() {
    libkrbn_initialize()
    libkrbn_load_custom_environment_variables()

    let userSettings = UserSettings()
    _userSettings = StateObject(wrappedValue: userSettings)

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }

    EVCoreServiceClient.shared.start()
    EVConsoleUserServerClient.shared.start()
    LibKrbn.FrontmostApplicationHistory.shared.watch()

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
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }
}
