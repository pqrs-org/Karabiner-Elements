import AppKit
import Foundation

private func versionUpdatedCallbackRelaunch() {
  Relauncher.relaunch()
}

private func versionUpdatedCallbackTerminate() {
  NSApplication.shared.terminate(nil)
}

final class KarabinerAppHelper: NSObject {
  public static let shared = KarabinerAppHelper()

  func observeVersionUpdated() {
    observeVersionUpdated(relaunch: true)
  }

  func observeVersionUpdated(relaunch: Bool) {
    libkrbn_enable_version_monitor()
    if relaunch {
      libkrbn_register_version_updated_callback(versionUpdatedCallbackRelaunch)
    } else {
      libkrbn_register_version_updated_callback(versionUpdatedCallbackTerminate)
    }
  }

  func observeConsoleUserServerIsDisabledNotification() {
    var buffer = [Int8](repeating: 0, count: 32 * 1024)

    libkrbn_get_distributed_notification_console_user_server_is_disabled(&buffer, buffer.count)
    let name = String(cString: buffer)

    libkrbn_get_distributed_notification_observed_object(&buffer, buffer.count)
    let object = String(cString: buffer)

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
      libkrbn_services_unregister_core_agents()
    }
  }
}
