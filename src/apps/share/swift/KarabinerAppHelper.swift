import AppKit
import Foundation

private func versionUpdatedCallbackRelaunch() {
  Relauncher.relaunch()
}

@MainActor
final class KarabinerAppHelper {
  public static let shared = KarabinerAppHelper()

  func observeVersionUpdated() {
    libkrbn_enable_version_monitor()
    libkrbn_register_version_updated_callback(versionUpdatedCallbackRelaunch)
  }

  func endAllAttachedSheets(_ window: NSWindow) {
    while let sheet = window.attachedSheet {
      endAllAttachedSheets(sheet)
      window.endSheet(sheet)
    }
  }

  enum QuitFrom {
    case menu
    case settings
  }

  func quitKarabiner(askForConfirmation: Bool, quitFrom: QuitFrom) {
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
        quitKarabiner(
          askForConfirmation: false,
          quitFrom: quitFrom)
      }
    } else {
      switch quitFrom {
      case .menu:
        libkrbn_killall_settings()
        libkrbn_services_unregister_all_agents()
      case .settings:
        libkrbn_services_unregister_all_agents()
        libkrbn_killall_settings()
      }
    }
  }
}
