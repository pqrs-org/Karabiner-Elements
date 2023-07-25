import AppKit
import Foundation

private func versionChangedCallback(_ context: UnsafeMutableRawPointer?) {
  Relauncher.relaunch()
}

final class KarabinerAppHelper: NSObject {
  public static let shared = KarabinerAppHelper()

  func observeVersionChange() {
    libkrbn_enable_version_monitor(versionChangedCallback, nil)
  }

  func observeConsoleUserServerIsDisabledNotification() {
    let name = String(
      cString: libkrbn_get_distributed_notification_console_user_server_is_disabled())
    let object = String(cString: libkrbn_get_distributed_notification_observed_object())

    DistributedNotificationCenter.default().addObserver(
      self,
      selector: #selector(consoleUserServerIsDisabledCallback),
      name: Notification.Name(name),
      object: object,
      suspensionBehavior: .deliverImmediately)
  }

  @objc
  private func consoleUserServerIsDisabledCallback() {
    Task { @MainActor in
      print("console_user_server is disabled")
      NSApplication.shared.terminate(self)
    }
  }

  func endAllAttachedSheets(_ window: NSWindow) {
    while let sheet = window.attachedSheet {
      endAllAttachedSheets(sheet)
      window.endSheet(sheet)
    }
  }

  func quitKarabiner(askForConfirmation: Bool) {
    if askForConfirmation {
      let alert = NSAlert()
      alert.messageText = "Are you sure you want to quit Karabiner-Elements?"
      alert.informativeText = "The changed key will be restored after Karabiner-Elements is quit."
      alert.addButton(withTitle: "Quit")
      alert.buttons[0].tag = NSApplication.ModalResponse.OK.rawValue
      alert.addButton(withTitle: "Cancel")
      alert.buttons[1].tag = NSApplication.ModalResponse.cancel.rawValue

      let result = alert.runModal()
      if result == .OK {
        quitKarabiner(askForConfirmation: false)
      }
    } else {
      libkrbn_launchctl_manage_console_user_server(false)
      libkrbn_launchctl_manage_notification_window(false)
    }
  }
}
