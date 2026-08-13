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

      mainWindow.contentView = NSHostingView(
        rootView: NotificationView(maximumWidth: .greatestFiniteMagnitude))
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
    guard screenWindows.count == screens.count else {
      updateWindows()
      return
    }

    for (index, window) in screenWindows.enumerated() {
      let settings = ConsoleUserServerUIState.shared.notificationWindowSettings
      let screen = screens[index]
      let frame: NSRect
      if settings.respectScreenVisibleFrame {
        frame = screen.visibleFrame
      } else {
        // Use the Dock area while keeping the notification below the menu bar.
        frame = NSRect(
          x: screen.frame.minX,
          y: screen.frame.minY,
          width: screen.frame.width,
          height: screen.visibleFrame.maxY - screen.frame.minY)
      }
      guard let contentView = window.contentView else {
        continue
      }

      if let hostingView = contentView as? NSHostingView<NotificationView> {
        let maximumWidth = max(1.0, frame.width - 20.0)
        if hostingView.rootView.maximumWidth != maximumWidth {
          hostingView.rootView = NotificationView(maximumWidth: maximumWidth)
        }
      }

      contentView.layoutSubtreeIfNeeded()
      let windowSize = contentView.fittingSize
      if window.contentLayoutRect.size != windowSize {
        window.setContentSize(windowSize)
      }
      let x: CGFloat
      let y: CGFloat

      switch settings.position {
      case .topLeft:
        x = frame.origin.x + 10.0
        y = frame.origin.y + frame.height - windowSize.height - 10.0
      case .topRight:
        x = frame.origin.x + frame.width - windowSize.width - 10.0
        y = frame.origin.y + frame.height - windowSize.height - 10.0
      case .bottomLeft:
        x = frame.origin.x + 10.0
        y = frame.origin.y + 10.0
      case .bottomRight:
        x = frame.origin.x + frame.width - windowSize.width - 10.0
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
