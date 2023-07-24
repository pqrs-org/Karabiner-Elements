import AppKit
import Foundation

private func versionChangedCallback(_ context: UnsafeMutableRawPointer?) {
  Relauncher.relaunch()
}

struct KarabinerAppHelper {
  static func observeVersionChange() {
    libkrbn_enable_version_monitor(versionChangedCallback, nil)
  }
}
