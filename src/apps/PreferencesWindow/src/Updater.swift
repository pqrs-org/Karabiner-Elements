#if USE_SPARKLE
    import Sparkle
#endif

@objc
public class Updater: NSObject {
    static func checkForUpdatesInBackground() {
        #if USE_SPARKLE
            let url = feedURL(false)
            print("checkForUpdates \(url)")

            SUUpdater.shared().feedURL = url
            SUUpdater.shared().checkForUpdatesInBackground()
        #endif
    }

    @objc
    static func checkForUpdatesStableOnly() {
        #if USE_SPARKLE
            let url = feedURL(false)
            print("checkForUpdates \(url)")

            SUUpdater.shared().feedURL = url
            SUUpdater.shared()?.checkForUpdates(self)
        #endif
    }

    @objc
    static func checkForUpdatesWithBetaVersion() {
        #if USE_SPARKLE
            let url = feedURL(true)
            print("checkForUpdates \(url)")

            SUUpdater.shared().feedURL = url
            SUUpdater.shared()?.checkForUpdates(self)
        #endif
    }

    @objc
    static func startTerminationTimer() -> Timer {
        return Timer.scheduledTimer(withTimeInterval: 1.0,
                                    repeats: true) { (_: Timer) in
            #if USE_SPARKLE
                if !SUUpdater.shared().updateInProgress {
                    NSApplication.shared.terminate(self)
                }
            #else
                NSApplication.shared.terminate(self)
            #endif
        }
    }

    private static func feedURL(_ includingBetaVersions: Bool) -> URL {
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
}
