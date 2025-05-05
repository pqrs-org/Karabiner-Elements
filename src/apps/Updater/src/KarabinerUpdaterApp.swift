import AppKit
import SwiftUI
import os

@main
struct KarabinerUpdaterApp: App {
  init() {
    libkrbn_initialize()

    //
    // Set it to automatically exit after completing the check.
    //

    NotificationCenter.default.addObserver(
      forName: Updater.didFinishUpdateCycleFor,
      object: nil,
      queue: .main
    ) { _ in
      libkrbn_terminate()
      NSApplication.shared.terminate(nil)
    }

    //
    // Process command-line arguments.
    //

    var command = ""
    #if USE_SPARKLE
      if CommandLine.arguments.count > 1 {
        command = CommandLine.arguments[1]
      }
    #endif

    switch command {
    case "checkForUpdatesInBackground":
      if !libkrbn_lock_single_application_with_user_pid_file(
        "check_for_updates_in_background.pid")
      {
        print("Exit since another process is running.")
        libkrbn_terminate()
        NSApplication.shared.terminate(nil)
      }

      Updater.shared.checkForUpdatesInBackground()

    case "checkForUpdatesStableOnly":
      Updater.shared.checkForUpdatesStableOnly()

    case "checkForUpdatesWithBetaVersion":
      Updater.shared.checkForUpdatesWithBetaVersion()

    default:
      libkrbn_terminate()
      NSApplication.shared.terminate(nil)
    }
  }

  var body: some Scene {
  }
}
