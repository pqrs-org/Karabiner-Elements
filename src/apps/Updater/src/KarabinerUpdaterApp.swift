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
  public func applicationDidFinishLaunching(_: Notification) {
    Task { @MainActor in
      var command = ""
      if CommandLine.arguments.count > 1 {
        command = CommandLine.arguments[1]
      }

      switch command {
      case "checkForUpdatesInBackground":
        Updater.shared.checkForUpdatesInBackground()

      case "checkForUpdatesStableOnly":
        Updater.shared.checkForUpdatesStableOnly()

      case "checkForUpdatesWithBetaVersion":
        Updater.shared.checkForUpdatesWithBetaVersion()

      default:
        NSApplication.shared.terminate(nil)
      }
    }
  }
}
