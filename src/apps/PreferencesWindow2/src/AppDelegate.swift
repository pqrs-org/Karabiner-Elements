import AppKit
import SwiftUI

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  @IBOutlet var simpleModificationsTableViewController: SimpleModificationsTableViewController!
  @IBOutlet var window: NSWindow!
  @IBOutlet var systemPreferencesManager: SystemPreferencesManager!
  @IBOutlet var stateJsonMonitor: StateJsonMonitor!
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

    systemPreferencesManager.setup()
    stateJsonMonitor.start()

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

              ContentViewStates.shared.complexModificationsSheetView =
                ComplexModificationsSheetView.fileImport
              ContentViewStates.shared.complexModificationsSheetPresented = true
              return
            }
          }
        }
      }

      if urlComponents?.path == "/simple_modifications/new" {
        if let queryItems = urlComponents?.queryItems {
          for pair in queryItems {
            if pair.name == "json" {
              self.simpleModificationsTableViewController.addItem(fromJson: pair.value)
              self.simpleModificationsTableViewController.openSimpleModificationsTab()
              return
            }
          }
        }
      }

      let alert = NSAlert()
      alert.messageText = "Error"
      alert.informativeText = "Unknown URL"
      alert.addButton(withTitle: "OK")

      alert.beginSheetModal(for: self.window) { _ in
      }
    }
  }
}
