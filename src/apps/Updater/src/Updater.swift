import AppKit

#if USE_SPARKLE
  import Sparkle
#endif

@MainActor
final class Updater: ObservableObject {
  public static let shared = Updater()

  #if USE_SPARKLE
    private let updaterController: SPUStandardUpdaterController
    private let delegate = SparkleDelegate()
  #endif

  private init() {
    #if USE_SPARKLE
      updaterController = SPUStandardUpdaterController(
        updaterDelegate: delegate,
        userDriverDelegate: nil
      )

      updaterController.updater.clearFeedURLFromUserDefaults()
    #endif
  }

  func checkForUpdatesInBackground() {
    #if USE_SPARKLE
      delegate.includingBetaVersions = false
      updaterController.updater.checkForUpdatesInBackground()
    #endif
  }

  func checkForUpdatesStableOnly() {
    #if USE_SPARKLE
      delegate.includingBetaVersions = false
      updaterController.checkForUpdates(nil)
    #endif
  }

  func checkForUpdatesWithBetaVersion() {
    #if USE_SPARKLE
      delegate.includingBetaVersions = true
      updaterController.checkForUpdates(nil)
    #endif
  }

  #if USE_SPARKLE
    private class SparkleDelegate: NSObject, SPUUpdaterDelegate {
      var includingBetaVersions = false

      func feedURLString(for updater: SPUUpdater) -> String? {
        var url = "https://appcast.pqrs.org/karabiner-elements-appcast.xml"
        if includingBetaVersions {
          url = "https://appcast.pqrs.org/karabiner-elements-appcast-devel.xml"
        }

        print("feedURLString \(url)")

        return url
      }

      func updaterShouldRelaunchApplication(_ updater: SPUUpdater) -> Bool {
        return false
      }

      func updaterDidNotFindUpdate(_: SPUUpdater) {
        Task { @MainActor in
          NSApp.activate(ignoringOtherApps: true)
        }
      }

      func updater(_: SPUUpdater, didFindValidUpdate _: SUAppcastItem) {
        Task { @MainActor in
          NSApp.activate(ignoringOtherApps: true)
        }
      }

      func updater(
        _: SPUUpdater, didFinishUpdateCycleFor _: SPUUpdateCheck, error: Error?
      ) {
        // Exit after completing the check.
        Task { @MainActor in
          NSApplication.shared.terminate(nil)
        }
      }
    }
  #endif
}
