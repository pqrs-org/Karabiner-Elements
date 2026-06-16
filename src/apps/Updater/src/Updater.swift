import AppKit
import OSLog
import Sparkle

@MainActor
final class Updater: ObservableObject {
  public static let shared = Updater()

  private let updaterController: SPUStandardUpdaterController
  private let delegate = SparkleDelegate()

  private init() {
    updaterController = SPUStandardUpdaterController(
      updaterDelegate: delegate,
      userDriverDelegate: nil
    )

    updaterController.updater.clearFeedURLFromUserDefaults()
  }

  isolated deinit {
    delegate.cancelUserAttentionRequest()
  }

  func checkForUpdatesInBackground() {
    delegate.includingBetaVersions = false
    updaterController.updater.checkForUpdatesInBackground()
  }

  func checkForUpdatesStableOnly() {
    delegate.includingBetaVersions = false
    updaterController.checkForUpdates(nil)
  }

  func checkForUpdatesWithBetaVersion() {
    delegate.includingBetaVersions = true
    updaterController.checkForUpdates(nil)
  }

  private class SparkleDelegate: NSObject, SPUUpdaterDelegate {
    private let logger = Logger(
      subsystem: Bundle.main.bundleIdentifier ?? "unknown",
      category: String(describing: SparkleDelegate.self))

    var includingBetaVersions = false
    private var userAttentionRequestIdentifier: Int?

    func feedURLString(for updater: SPUUpdater) -> String? {
      var url = "https://appcast.pqrs.org/karabiner-elements-appcast.xml"
      if includingBetaVersions {
        url = "https://appcast.pqrs.org/karabiner-elements-appcast-devel.xml"
      }

      logger.info("feedURLString \(url, privacy: .public)")

      return url
    }

    func updaterShouldRelaunchApplication(_ updater: SPUUpdater) -> Bool {
      return false
    }

    func updaterDidNotFindUpdate(_: SPUUpdater) {
      Task { @MainActor in
        cancelUserAttentionRequest()
      }
    }

    func updater(_: SPUUpdater, didFindValidUpdate _: SUAppcastItem) {
      Task { @MainActor in
        // Just in case, wait until the update notification window is shown.
        try await Task.sleep(for: .seconds(1))

        if userAttentionRequestIdentifier == nil {
          userAttentionRequestIdentifier = NSApp.requestUserAttention(.criticalRequest)
        }

        NSApp.dockTile.badgeLabel = "!"
      }
    }

    func updater(
      _: SPUUpdater, didFinishUpdateCycleFor _: SPUUpdateCheck, error: Error?
    ) {
      // Exit after completing the check.
      Task { @MainActor in
        cancelUserAttentionRequest()
        NSApplication.shared.terminate(nil)
      }
    }

    @MainActor
    func cancelUserAttentionRequest() {
      if let identifier = userAttentionRequestIdentifier {
        NSApp.cancelUserAttentionRequest(identifier)
        userAttentionRequestIdentifier = nil
      }

      NSApp.dockTile.badgeLabel = nil
    }
  }
}
