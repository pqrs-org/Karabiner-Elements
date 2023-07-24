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
}
