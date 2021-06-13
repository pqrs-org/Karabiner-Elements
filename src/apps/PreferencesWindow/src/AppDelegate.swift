import AppKit

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    @IBOutlet var simpleModificationsTableViewController: SimpleModificationsTableViewController!
    @IBOutlet var complexModificationsFileImportWindowController: ComplexModificationsFileImportWindowController!
    @IBOutlet var preferencesWindow: NSWindow!
    @IBOutlet var preferencesWindowController: PreferencesWindowController!
    @IBOutlet var systemPreferencesManager: SystemPreferencesManager!
    @IBOutlet var stateJsonMonitor: StateJsonMonitor!

    override public init() {
        super.init()
        libkrbn_initialize()
    }

    public func applicationWillFinishLaunching(_: Notification) {
        NSAppleEventManager.shared().setEventHandler(self,
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
        preferencesWindowController.setup()
        stateJsonMonitor.start()

        NotificationCenter.default.addObserver(forName: Updater.didFindValidUpdate,
                                               object: nil,
                                               queue: .main) { [weak self] _ in
            guard let self = self else { return }

            self.preferencesWindowController.show()
        }

        NotificationCenter.default.addObserver(forName: Updater.updaterDidNotFindUpdate,
                                               object: nil,
                                               queue: .main) { _ in
            NSApplication.shared.terminate(nil)
        }

        //
        // Run updater or open preferences.
        //

        if CommandLine.arguments.count > 1 {
            let command = CommandLine.arguments[1]
            switch command {
            case "checkForUpdatesInBackground":
                #if USE_SPARKLE
                    Updater.shared.checkForUpdatesInBackground()
                    return
                #else
                    NSApplication.shared.terminate(self)
                #endif
            case "checkForUpdatesStableOnly":
                #if USE_SPARKLE
                    Updater.shared.checkForUpdatesStableOnly()
                    return
                #else
                    NSApplication.shared.terminate(self)
                #endif
            case "checkForUpdatesWithBetaVersion":
                #if USE_SPARKLE
                    Updater.shared.checkForUpdatesWithBetaVersion()
                    return
                #else
                    NSApplication.shared.terminate(self)
                #endif
            default:
                break
            }
        }

        preferencesWindowController.show()
    }

    public func applicationWillTerminate(_: Notification) {
        libkrbn_terminate()
    }

    public func applicationShouldHandleReopen(_: NSApplication, hasVisibleWindows _: Bool) -> Bool {
        preferencesWindowController.show()
        return true
    }

    @objc func handleGetURLEvent(_ event: NSAppleEventDescriptor,
                                 withReplyEvent _: NSAppleEventDescriptor)
    {
        // - url == "karabiner://karabiner/assets/complex_modifications/import?url=xxx"
        // - url == "karabiner://karabiner/simple_modifications/new?json={xxx}"
        guard let url = event.paramDescriptor(forKeyword: AEKeyword(keyDirectObject))?.stringValue else { return }

        KarabinerKit.endAllAttachedSheets(preferencesWindow)

        let urlComponents = URLComponents(string: url)

        if urlComponents?.path == "/assets/complex_modifications/import" {
            if let queryItems = urlComponents?.queryItems {
                for pair in queryItems {
                    if pair.name == "url" {
                        complexModificationsFileImportWindowController.setup(pair.value)
                        complexModificationsFileImportWindowController.show()
                        return
                    }
                }
            }
        }

        if urlComponents?.path == "/simple_modifications/new" {
            if let queryItems = urlComponents?.queryItems {
                for pair in queryItems {
                    if pair.name == "json" {
                        simpleModificationsTableViewController.addItem(fromJson: pair.value)
                        simpleModificationsTableViewController.openSimpleModificationsTab()
                        return
                    }
                }
            }
        }

        let alert = NSAlert()
        alert.messageText = "Error"
        alert.informativeText = "Unknown URL"
        alert.addButton(withTitle: "OK")

        alert.beginSheetModal(for: preferencesWindow) { _ in
        }
    }
}
