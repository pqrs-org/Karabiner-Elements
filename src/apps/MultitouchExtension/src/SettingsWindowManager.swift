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

    //
    // Calculate the window size
    //

    let contentView = NSHostingView(rootView: SettingsView())
    contentView.translatesAutoresizingMaskIntoConstraints = false
    contentView.layoutSubtreeIfNeeded()
    let size = contentView.fittingSize

    //
    // Create a window
    //

    let window = NSWindow(
      contentRect: NSRect(origin: .zero, size: size),
      styleMask: [
        .titled,
        .closable,
        .miniaturizable,
        .fullSizeContentView,
      ],
      backing: .buffered,
      defer: false
    )

    window.isReleasedWhenClosed = false
    window.title = "Karabiner-MultitouchExtension Settings"
    window.contentView = contentView
    window.delegate = self
    window.center()

    window.makeKeyAndOrderFront(self)
    NSApp.activate(ignoringOtherApps: true)

    settingsWindow = window
  }
}

extension SettingsWindowManager: NSWindowDelegate {
  func windowWillClose(_: Notification) {
    closed = true
  }
}
