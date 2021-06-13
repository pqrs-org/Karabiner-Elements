import AppKit

#if USE_SPARKLE
    import Sparkle
#endif

@objc
public class Updater: NSObject {
    @objc
    public static let shared = Updater()
    @objc
    public static let didFindValidUpdate = Notification.Name("didFindValidUpdate")
    @objc
    public static let updaterDidNotFindUpdate = Notification.Name("updaterDidNotFindUpdate")

    func checkForUpdatesInBackground() {
        #if USE_SPARKLE
            let url = feedURL(false)
            print("checkForUpdates \(url)")

            SUUpdater.shared().feedURL = url
            SUUpdater.shared().delegate = self
            SUUpdater.shared().checkForUpdatesInBackground()
        #endif
    }

    @objc
    func checkForUpdatesStableOnly() {
        #if USE_SPARKLE
            let url = feedURL(false)
            print("checkForUpdates \(url)")

            SUUpdater.shared().feedURL = url
            SUUpdater.shared().delegate = self
            SUUpdater.shared()?.checkForUpdates(self)
        #endif
    }

    @objc
    func checkForUpdatesWithBetaVersion() {
        #if USE_SPARKLE
            let url = feedURL(true)
            print("checkForUpdates \(url)")

            SUUpdater.shared().feedURL = url
            SUUpdater.shared().delegate = self
            SUUpdater.shared()?.checkForUpdates(self)
        #endif
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
}

#if USE_SPARKLE
    extension Updater: SUUpdaterDelegate {
        public func updater(_: SUUpdater, didFindValidUpdate _: SUAppcastItem) {
            NotificationCenter.default.post(name: Updater.didFindValidUpdate, object: nil)
        }

        public func updaterDidNotFindUpdate(_: SUUpdater) {
            NotificationCenter.default.post(name: Updater.updaterDidNotFindUpdate, object: nil)
        }
    }
#endif
