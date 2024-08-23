import Cocoa
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  var window: NSWindow?

  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    if !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent) {
      InputMonitoringAlertData.shared.showing = true
    }

    window = NSWindow(
      contentRect: .zero,
      styleMask: [
        .titled,
        .closable,
        .miniaturizable,
        .resizable,
        .fullSizeContentView,
      ],
      backing: .buffered,
      defer: false
    )
    window!.title = "Karabiner-EventViewer"
    window!.contentView = NSHostingView(rootView: ContentView())
    window!.center()
    window!.makeKeyAndOrderFront(self)

    setWindowProperty()

    NotificationCenter.default.addObserver(
      forName: UserSettings.windowSettingChanged,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      self.setWindowProperty()
    }

    NSEvent.addLocalMonitorForEvents(matching: .keyDown) { event -> NSEvent? in
      if event.modifierFlags.intersection(.deviceIndependentFlagsMask) == .command {
        if event.charactersIgnoringModifiers == "q" || event.charactersIgnoringModifiers == "w" {
          if UserSettings.shared.quitUsingKeyboardShortcut {
            NSApplication.shared.terminate(nil)
          }
        }
      }
      return event
    }

    DevicesJsonString.shared.start()
    EventHistory.shared.start()
    FrontmostApplicationHistory.shared.start()
    VariablesJsonString.shared.start()
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    true
  }

  private func setWindowProperty() {
    if let window = window {
      // ----------------------------------------
      if UserSettings.shared.forceStayTop {
        window.level = .floating
      } else {
        window.level = .normal
      }

      // ----------------------------------------
      if UserSettings.shared.showInAllSpaces {
        window.collectionBehavior.insert(.canJoinAllSpaces)
      } else {
        window.collectionBehavior.remove(.canJoinAllSpaces)
      }

      window.collectionBehavior.insert(.managed)
      window.collectionBehavior.remove(.moveToActiveSpace)
      window.collectionBehavior.remove(.transient)
    }
  }
}

extension AppDelegate: NSWindowDelegate {
  public func windowWillClose(_: Notification) {
    NSApplication.shared.terminate(self)
  }
}
