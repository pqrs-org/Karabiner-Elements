import AppKit
import Sparkle

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    private var timer: Timer?

    public func applicationDidFinishLaunching(_: Notification) {
        ProcessInfo.processInfo.enableSuddenTermination()

        var mode = "checkForUpdatesInBackground"

        let arguments = ProcessInfo.processInfo.arguments
        if arguments.count == 2 {
            mode = arguments[1]
        }

        print("mode \(mode)")

        if mode == "checkForUpdatesStableOnly" {
            checkForUpdatesStableOnly()
        } else if mode == "checkForUpdatesWithBetaVersion" {
            checkForUpdatesWithBetaVersion()
        } else {
            checkForUpdatesInBackground()
        }
    }

    private func checkForUpdatesInBackground() {
        let url = feedURL(false)
        print("checkForUpdates \(url)")

        SUUpdater.shared().feedURL = url
        SUUpdater.shared().checkForUpdatesInBackground()

        setTerminationTimer()
    }

    private func checkForUpdatesStableOnly() {
        let url = feedURL(false)
        print("checkForUpdates \(url)")

        SUUpdater.shared().feedURL = url
        SUUpdater.shared()?.checkForUpdates(self)

        setTerminationTimer()
    }

    private func checkForUpdatesWithBetaVersion() {
        let url = feedURL(true)
        print("checkForUpdates \(url)")

        SUUpdater.shared().feedURL = url
        SUUpdater.shared()?.checkForUpdates(self)

        setTerminationTimer()
    }

    private func feedURL(_ includingBetaVersions: Bool) -> URL {
        // ----------------------------------------
        // check beta & stable releases.

        // Once we check appcast.xml, SUFeedURL is stored in a user's preference file.
        // So that Sparkle gives priority to a preference over Info.plist,
        // we overwrite SUFeedURL here.
        if includingBetaVersions {
            return URL(string: "https://appcast.pqrs.org/karabiner-elements-appcast-devel.xml")!
        }

        return URL(string: "https://appcast.pqrs.org/karabiner-elements-appcast.xml")!
    }

    private func setTerminationTimer() {
        timer = Timer.scheduledTimer(withTimeInterval: 1.0,
                                     repeats: true) { (_: Timer) in
            if !SUUpdater.shared().updateInProgress {
                NSApplication.shared.terminate(self)
            }
        }
    }
}
