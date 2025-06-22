import SwiftUI

private struct NotificationMessageJson: Codable {
  var body: String?
}

private func callback() {
  Task { @MainActor in
    var body = ""

    if let jsonData = try? Data(
      contentsOf: URL(
        fileURLWithPath: NotificationWindowManager.shared.notificationMessageJsonFilePath))
    {
      let decoder = JSONDecoder()
      decoder.keyDecodingStrategy = .convertFromSnakeCase
      if let message = try? decoder.decode(NotificationMessageJson.self, from: jsonData) {
        body = message.body ?? ""
        body = body.trimmingCharacters(in: .whitespacesAndNewlines)
      }
    }

    NotificationMessage.shared.body = body
    NotificationWindowManager.shared.updateWindowsVisibility()
  }
}

@MainActor
public class NotificationWindowManager: NSObject {
  static let shared = NotificationWindowManager()

  let notificationMessageJsonFilePath = LibKrbn.notificationMessageJsonFilePath()

  private var notificationsTask: Task<Void, Never>?

  private struct ScreenWindow {
    var mainWindow: NSWindow
    var closeButtonWindow: NSWindow
  }

  private var screenWindows: [ScreenWindow] = []

  override public init() {
    super.init()

    notificationsTask = Task {
      await withTaskGroup(of: Void.self) { group in
        group.addTask {
          for await _ in NotificationCenter.default.notifications(
            named: NSApplication.didChangeScreenParametersNotification
          ) {
            await self.updateWindows()
          }
        }
      }
    }
  }

  deinit { notificationsTask?.cancel() }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_file_monitors()

    libkrbn_register_file_updated_callback(
      notificationMessageJsonFilePath.cString(using: .utf8),
      callback)
    libkrbn_enqueue_callback(callback)

    updateWindows()
    updateWindowsVisibility()
  }

  public func stop() {
    libkrbn_unregister_file_updated_callback(
      notificationMessageJsonFilePath.cString(using: .utf8),
      callback)

    // We don't call `libkrbn_disable_file_monitors` because the file monitors may be used elsewhere.
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
    let hide = NotificationMessage.shared.body.isEmpty
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
