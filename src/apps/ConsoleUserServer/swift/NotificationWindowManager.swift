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
      let windowSize = window.contentView?.fittingSize ?? NSSize(width: 410.0, height: 70.0)
      let position = ConsoleUserServerUIState.shared.notificationWindowSettings.position
      let x = frame.origin.x + frame.width - windowSize.width
      let y: CGFloat

      if position == "top_right" {
        y = frame.origin.y + frame.height - windowSize.height - 10.0
      } else {
        y = frame.origin.y + 10.0
      }

      window.setFrameOrigin(
        NSPoint(
          x: x,
          y: y))
    }
  }

  func updateWindowsVisibility() {
    let state = ConsoleUserServerUIState.shared
    let hide =
      !state.configurationLoaded || !state.notificationWindowSettings.enabled
      || state.notificationMessage.isEmpty

    if !hide {
      updateWindowsFrameOrigin()
    }

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
