import SettingsAccess
import SwiftUI

@main
struct KarabinerMultitouchExtensionApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  private let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

  init() {
    libkrbn_initialize()
  }

  var body: some Scene {
    MenuBarExtra(
      "Karabiner-MultitouchExtension", systemImage: "rectangle.and.hand.point.up.left.filled",
    ) {
      Text("Karabiner-MultitouchExtension \(version)")

      Divider()

      SettingsLink {
        Label("Settings...", systemImage: "gearshape")
          .labelStyle(.titleAndIcon)
      } preAction: {
        NSApp.activate(ignoringOtherApps: true)
      } postAction: {
      }
    }

    Settings {
      SettingsView()
    }
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  private var activity: NSObjectProtocol?

  public func applicationDidFinishLaunching(_: Notification) {
    //
    // Enable grabber_client
    //

    MEGrabberClient.shared.start()
    MultitouchDeviceManager.shared.observeIONotification()

    //
    // Disable App Nap
    //

    activity = ProcessInfo.processInfo.beginActivity(
      options: .userInitiated,
      reason: "Disable App Nap in order to receive multitouch events even if this app is background"
    )
  }

  public func applicationWillTerminate(_: Notification) {
    if let a = activity {
      ProcessInfo.processInfo.endActivity(a)
      activity = nil
    }

    MultitouchDeviceManager.shared.setCallback(false)

    libkrbn_terminate()
  }
}
