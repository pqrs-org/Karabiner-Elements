import AppKit
import SwiftUI
import os

// Note #1:
// The icon shown in Sparkle's dialog is generally taken from the file specified by CFBundleIconFile.
// As a result, the icon overridden by AppIconSwitcher is not reflected.
// The default icon is always used instead.

// Note #2:
// If an update is available, the Updater may attempt to restart during the update process.
// When using libkrbn, this can cause the process to abort where libkrbn_terminate is not called,
// Therefore, please avoid using libkrbn in Updater.

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
      #if USE_SPARKLE
        if CommandLine.arguments.count > 1 {
          command = CommandLine.arguments[1]
        }
      #endif

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
