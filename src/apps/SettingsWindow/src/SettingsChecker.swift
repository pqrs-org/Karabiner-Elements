import Combine
import Foundation
import SwiftUI

@MainActor
final class SettingsChecker: ObservableObject {
  static let shared = SettingsChecker()

  @ObservedObject private var settings = LibKrbn.Settings.shared
  @Published var keyboardTypeEmpty = false
  private var subscribers: Set<AnyCancellable> = []

  public func start() {
    settings.$virtualHIDKeyboardKeyboardTypeV2.sink { _ in
      Task { @MainActor in
        // Add a delay to prevent the alert from briefly appearing at startup.
        do {
          try await Task.sleep(nanoseconds: 100 * NSEC_PER_MSEC)
        } catch {
          print(error.localizedDescription)
        }

        self.check()
      }
    }.store(in: &subscribers)
  }

  private func check() {
    keyboardTypeEmpty = (settings.virtualHIDKeyboardKeyboardTypeV2 == "")
    updateShowSettingsAlert()
  }

  private func updateShowSettingsAlert() {
    if keyboardTypeEmpty {
      ContentViewStates.shared.showSettingsAlert = true
    } else {
      ContentViewStates.shared.showSettingsAlert = false
    }
  }
}
