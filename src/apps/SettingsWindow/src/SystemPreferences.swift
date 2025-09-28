import AsyncAlgorithms
import Combine
import Foundation
import SwiftUI

@MainActor
final class SystemPreferences: ObservableObject {
  static let shared = SystemPreferences()

  @Published var virtualHIDKeyboardModifierMappingsExists: Bool = false

  private let timer: AsyncTimerSequence<ContinuousClock>
  private var timerTask: Task<Void, Never>?

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  init() {
    timer = AsyncTimerSequence(
      interval: .seconds(3),
      clock: .continuous
    )
  }

  public func start() {
    //
    // Start the timer
    //

    timerTask = Task { @MainActor in
      self.checkModifierMappings()

      for await _ in timer {
        self.checkModifierMappings()
      }
    }
  }

  public func stop() {
    //
    // Stop the timer
    //

    timerTask?.cancel()
  }

  private func checkModifierMappings() {
    virtualHIDKeyboardModifierMappingsExists =
      libkrbn_system_preferences_virtual_hid_keyboard_modifier_mappings_exists()
  }
}
