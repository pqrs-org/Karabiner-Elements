import SwiftUI

class SettingsWindowManager: NSObject {
  static let shared = SettingsWindowManager()

  private var settingsWindow: NSWindow?
  private var closed = false

  func show() {
    if settingsWindow != nil, !closed {
      settingsWindow!.makeKeyAndOrderFront(self)
      NSApp.activate(ignoringOtherApps: true)
      return
    }

    closed = false

    settingsWindow = NSWindow(
      contentRect: .zero,
      styleMask: [
        .titled,
        .closable,
        .miniaturizable,
        .fullSizeContentView,
      ],
      backing: .buffered,
      defer: false
    )

    settingsWindow!.isReleasedWhenClosed = false
    settingsWindow!.title = "Karabiner-MultitouchExtension Settings"
    settingsWindow!.contentView = NSHostingView(rootView: SettingsView())
    settingsWindow!.delegate = self
    settingsWindow!.center()

    settingsWindow!.makeKeyAndOrderFront(self)
    NSApp.activate(ignoringOtherApps: true)
  }
}

extension SettingsWindowManager: NSWindowDelegate {
  func windowWillClose(_: Notification) {
    closed = true
  }
}
