import SwiftUI

@MainActor
final class NotificationWindowManager: NSObject {
  static let shared = NotificationWindowManager()

  private var screenWindows: [NSWindow] = []
  private var notificationsTask: Task<Void, Never>?

  override init() {
    super.init()

    notificationsTask = Task {
      for await _ in NotificationCenter.default.notifications(
        named: NSApplication.didChangeScreenParametersNotification)
      {
        updateWindows()
      }
    }

    updateWindows()
  }

  deinit { notificationsTask?.cancel() }

  func updateWindows() {
    let screens = NSScreen.screens

    while screenWindows.count < screens.count {
      let mainWindow = NSWindow(
        contentRect: .zero,
        styleMask: [.fullSizeContentView],
        backing: .buffered,
        defer: false)

      mainWindow.contentView = NSHostingView(rootView: NotificationView())
      mainWindow.backgroundColor = .clear
      mainWindow.isOpaque = false
      mainWindow.level = .statusBar
      mainWindow.ignoresMouseEvents = true
      mainWindow.collectionBehavior = [.canJoinAllSpaces, .ignoresCycle]
      mainWindow.delegate = self

      screenWindows.append(mainWindow)
    }

    if screenWindows.count > screens.count {
      screenWindows.removeLast(screenWindows.count - screens.count)
    }

    updateWindowsFrameOrigin(screens)
    updateWindowsVisibility()
  }

  func updateWindowsFrameOrigin(_ screens: [NSScreen] = NSScreen.screens) {
    for (index, window) in screenWindows.enumerated() {
      let frame = screens[index].visibleFrame
      window.setFrameOrigin(
        NSPoint(
          x: frame.origin.x + frame.width - 410.0,
          y: frame.origin.y + 10.0))
    }
  }

  func updateWindowsVisibility() {
    let state = ConsoleUserServerUIState.shared
    let hide =
      !state.configurationLoaded || !state.notificationWindowSettings.enabled
      || state.notificationMessage.isEmpty
    for window in screenWindows {
      if hide {
        window.orderOut(self)
      } else {
        window.orderFront(self)
      }
    }
  }
}

extension NotificationWindowManager: NSWindowDelegate {
  func windowDidResize(_: Notification) {
    updateWindowsFrameOrigin()
  }
}
