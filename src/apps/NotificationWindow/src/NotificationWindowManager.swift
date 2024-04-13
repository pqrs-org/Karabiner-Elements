import SwiftUI

private func callback(
  _ filePath: UnsafePointer<Int8>?,
  _ context: UnsafeMutableRawPointer?
) {

  Task { @MainActor in
    var buffer = [Int8](repeating: 0, count: 32 * 1024)
    var body = ""

    if libkrbn_get_notification_message_body(&buffer, buffer.count) {
      body = String(cString: buffer)
    }

    NotificationMessage.shared.text = body.trimmingCharacters(in: .whitespacesAndNewlines)
    NotificationWindowManager.shared.updateWindowsVisibility()
  }
}

public class NotificationWindowManager: NSObject {
  static let shared = NotificationWindowManager()

  private struct ScreenWindow {
    var mainWindow: NSWindow
    var closeButtonWindow: NSWindow
  }

  private var screenWindows: [ScreenWindow] = []

  override public init() {
    super.init()

    NotificationCenter.default.addObserver(
      forName: NSApplication.didChangeScreenParametersNotification,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      self.updateWindows()
    }

    updateWindows()

    libkrbn_enable_notification_message_json_file_monitor(callback, nil)
  }

  deinit {
    libkrbn_disable_notification_message_json_file_monitor()
  }

  func updateWindows() {
    let screens = NSScreen.screens

    //
    // Create windows
    //
    // Note:
    // Do not release existing windows even if screens.count < screenWindows.count to avoid a high CPU usage issue.
    //

    while screenWindows.count < screens.count {
      let screenWindow = ScreenWindow(
        mainWindow: NSWindow(
          contentRect: .zero,
          styleMask: [
            .fullSizeContentView
          ],
          backing: .buffered,
          defer: false
        ),
        closeButtonWindow: NSWindow(
          contentRect: .zero,
          styleMask: [
            .fullSizeContentView
          ],
          backing: .buffered,
          defer: false
        )
      )

      //
      // Main window
      //

      screenWindow.mainWindow.contentView = NSHostingView(rootView: MainView())
      screenWindow.mainWindow.backgroundColor = NSColor.clear
      screenWindow.mainWindow.isOpaque = false
      screenWindow.mainWindow.level = .statusBar
      screenWindow.mainWindow.ignoresMouseEvents = true
      screenWindow.mainWindow.collectionBehavior.insert(.canJoinAllSpaces)
      screenWindow.mainWindow.collectionBehavior.insert(.ignoresCycle)
      screenWindow.mainWindow.delegate = self
      // screenWindow.mainWindow.collectionBehavior.insert(.stationary)

      //
      // Close button
      //

      screenWindow.closeButtonWindow.contentView = NSHostingView(
        rootView: ButtonView(
          mainWindow: screenWindow.mainWindow,
          buttonWindow: screenWindow.closeButtonWindow))
      screenWindow.closeButtonWindow.backgroundColor = NSColor.clear
      screenWindow.closeButtonWindow.isOpaque = false
      screenWindow.closeButtonWindow.level = .statusBar
      screenWindow.closeButtonWindow.ignoresMouseEvents = false
      screenWindow.closeButtonWindow.collectionBehavior.insert(.canJoinAllSpaces)
      screenWindow.closeButtonWindow.collectionBehavior.insert(.ignoresCycle)
      // screenWindow.closeButtonWindow.collectionBehavior.insert(.stationary)

      screenWindows.append(screenWindow)
    }

    updateWindowsFrameOrigin()
    updateWindowsVisibility()
  }

  func updateWindowsFrameOrigin() {
    let screens = NSScreen.screens

    for (i, screenWindow) in screenWindows.enumerated() {
      var screenFrame = NSRect.zero
      if i < screens.count {
        screenFrame = screens[i].visibleFrame

        screenWindow.mainWindow.setFrameOrigin(
          NSPoint(
            x: screenFrame.origin.x + screenFrame.size.width - 410,
            y: screenFrame.origin.y + 10
          ))

        screenWindow.closeButtonWindow.setFrame(
          NSRect(
            x: screenWindow.mainWindow.frame.origin.x - 8,
            y: screenWindow.mainWindow.frame.origin.y + screenWindow.mainWindow.frame.size.height
              - 16,
            width: CGFloat(24.0),
            height: CGFloat(24.0)),
          display: false)
      }
    }
  }

  func updateWindowsVisibility() {
    let hide = NotificationMessage.shared.text.isEmpty
    let screens = NSScreen.screens

    for (i, screenWindow) in screenWindows.enumerated() {
      if hide || i >= screens.count {
        screenWindow.mainWindow.orderOut(self)
        screenWindow.closeButtonWindow.orderOut(self)
      } else {
        screenWindow.mainWindow.orderFront(self)
        screenWindow.closeButtonWindow.orderFront(self)
      }
    }
  }
}

extension NotificationWindowManager: NSWindowDelegate {
  public func windowDidResize(_ notification: Notification) {
    updateWindowsFrameOrigin()
  }
}
