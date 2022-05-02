import AppKit
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?
  private var updaterMode = false

  override public init() {
    super.init()
    libkrbn_initialize()
  }

  public func applicationWillFinishLaunching(_: Notification) {
    NSAppleEventManager.shared().setEventHandler(
      self,
      andSelector: #selector(handleGetURLEvent(_:withReplyEvent:)),
      forEventClass: AEEventClass(kInternetEventClass),
      andEventID: AEEventID(kAEGetURL))
  }

  public func applicationDidFinishLaunching(_: Notification) {
    NSApplication.shared.disableRelaunchOnLogin()

    ProcessInfo.processInfo.enableSuddenTermination()

    KarabinerKit.setup()
    KarabinerKit.observeConsoleUserServerIsDisabledNotification()

    NotificationCenter.default.addObserver(
      forName: Updater.didFindValidUpdate,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      self.window!.makeKeyAndOrderFront(self)
      NSApp.activate(ignoringOtherApps: true)
    }

    NotificationCenter.default.addObserver(
      forName: Updater.updaterDidNotFindUpdate,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      if self.updaterMode {
        NSApplication.shared.terminate(nil)
      }
    }

    //
    // Run updater or open preferences.
    //

    if CommandLine.arguments.count > 1 {
      let command = CommandLine.arguments[1]
      switch command {
      case "checkForUpdatesInBackground":
        #if USE_SPARKLE
          updaterMode = true
          Updater.shared.checkForUpdatesInBackground()
          return
        #else
          NSApplication.shared.terminate(self)
        #endif
      case "checkForUpdatesStableOnly":
        #if USE_SPARKLE
          updaterMode = true
          Updater.shared.checkForUpdatesStableOnly()
          return
        #else
          NSApplication.shared.terminate(self)
        #endif
      case "checkForUpdatesWithBetaVersion":
        #if USE_SPARKLE
          updaterMode = true
          Updater.shared.checkForUpdatesWithBetaVersion()
          return
        #else
          NSApplication.shared.terminate(self)
        #endif
      default:
        break
      }
    }

    var psn = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
    TransformProcessType(
      &psn, ProcessApplicationTransformState(kProcessTransformToForegroundApplication))

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
    window!.title = "Karabiner-Elements Preferences"
    window!.contentView = NSHostingView(rootView: ContentView())
    window!.center()
    window!.makeKeyAndOrderFront(self)

    window!.makeKeyAndOrderFront(self)
    NSApp.activate(ignoringOtherApps: true)

    //
    // Start StateJsonMonitor
    //

    AlertWindowsManager.shared.parentWindow = window
    StateJsonMonitor.shared.start()

    //
    // launchctl
    //

    libkrbn_launchctl_manage_session_monitor()
    libkrbn_launchctl_manage_console_user_server(true)
    // Do not manage grabber_agent and observer_agent because they are designed to run only once.
  }

  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    if Updater.shared.updateInProgress {
      return false
    }
    return true
  }

  @objc func handleGetURLEvent(
    _ event: NSAppleEventDescriptor,
    withReplyEvent _: NSAppleEventDescriptor
  ) {
    // - url == "karabiner://karabiner/assets/complex_modifications/import?url=xxx"
    // - url == "karabiner://karabiner/simple_modifications/new?json={xxx}"
    guard let url = event.paramDescriptor(forKeyword: AEKeyword(keyDirectObject))?.stringValue
    else { return }

    DispatchQueue.main.async { [weak self] in
      guard let self = self else { return }

      KarabinerKit.endAllAttachedSheets(self.window)

      let urlComponents = URLComponents(string: url)

      if urlComponents?.path == "/assets/complex_modifications/import" {
        if let queryItems = urlComponents?.queryItems {
          for pair in queryItems {
            if pair.name == "url" {
              ComplexModificationsFileImport.shared.fetchJson(URL(string: pair.value!)!)

              ContentViewStates.shared.navigationSelection =
                NavigationTag.complexModifications.rawValue

              ContentViewStates.shared.complexModificationsViewSheetView =
                ComplexModificationsSheetView.fileImport
              ContentViewStates.shared.complexModificationsViewSheetPresented = true
              return
            }
          }
        }
      }

      if urlComponents?.path == "/simple_modifications/new" {
        if let queryItems = urlComponents?.queryItems {
          for pair in queryItems {
            if pair.name == "json" {
              if let jsonString = pair.value {
                ContentViewStates.shared.navigationSelection =
                  NavigationTag.simpleModifications.rawValue

                Settings.shared.appendSimpleModification(
                  jsonString: jsonString,
                  device: ContentViewStates.shared.simpleModificationsViewSelectedDevice)
                return
              }
            }
          }
        }
      }

      if let window = self.window {
        let alert = NSAlert()
        alert.messageText = "Error"
        alert.informativeText = "Unknown URL"
        alert.addButton(withTitle: "OK")

        alert.beginSheetModal(for: window) { _ in
        }
      }
    }
  }
}
