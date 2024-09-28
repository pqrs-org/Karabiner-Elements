import Combine
import Foundation
import SwiftUI

final class SettingsChecker: ObservableObject {
  static let shared = SettingsChecker()

  @ObservedObject private var settings = LibKrbn.Settings.shared
  @Published var keyboardTypeEmpty = false
  private var subscribers: Set<AnyCancellable> = []

  public func start() {
    settings.$virtualHIDKeyboardKeyboardTypeV2.sink { [weak self] newValue in
      self?.checkVirtualHIDKeyboardKeyboardTypeV2(newValue)
    }.store(in: &subscribers)
  }

  private func checkVirtualHIDKeyboardKeyboardTypeV2(_ virtualHIDKeyboardKeyboardTypeV2: String) {
    keyboardTypeEmpty = (virtualHIDKeyboardKeyboardTypeV2 == "")
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
