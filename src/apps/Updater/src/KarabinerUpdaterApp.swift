import AppKit
import SwiftUI
import os

@main
struct KarabinerUpdaterApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  var body: some Scene {
    // Provide an empty Settings to prevent build errors.
    Settings {}
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  private enum Command: String {
    case checkForUpdatesInBackground
    case checkForUpdatesStableOnly
    case checkForUpdatesWithBetaVersion
  }

  public func applicationDidFinishLaunching(_: Notification) {
    Task { @MainActor in
      let command = Command(
        rawValue: CommandLine.arguments.dropFirst().first ?? ""
      )

      switch command {
      case .checkForUpdatesInBackground:
        Updater.shared.checkForUpdatesInBackground()

      case .checkForUpdatesStableOnly:
        // Changing the activation policy does not bring the application to the front, and activation
        // before applicationDidFinishLaunching may be unreliable. Perform both operations here.
        NSApp.setActivationPolicy(.regular)
        NSApp.activate(ignoringOtherApps: true)
        Updater.shared.checkForUpdatesStableOnly()

      case .checkForUpdatesWithBetaVersion:
        NSApp.setActivationPolicy(.regular)
        NSApp.activate(ignoringOtherApps: true)
        Updater.shared.checkForUpdatesWithBetaVersion()

      case nil:
        NSApplication.shared.terminate(nil)
      }
    }
  }
}
