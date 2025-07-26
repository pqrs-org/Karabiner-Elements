import AsyncAlgorithms
import Foundation
import SwiftUI

@MainActor
final class Doctor: ObservableObject {
  static let shared = Doctor()

  private let timer: AsyncTimerSequence<ContinuousClock>
  private var timerTask: Task<Void, Never>?

  private var previousShowAlert: Bool?

  @Published var userPIDDirectoryWritable: Bool = true
  @Published var karabinerJSONParseErrorMessage = ""

  init() {
    timer = AsyncTimerSequence(
      interval: .seconds(3),
      clock: .continuous
    )
  }

  public func start() {
    timerTask = Task { @MainActor in
      self.check()

      for await _ in timer {
        self.check()
      }
    }
  }

  public func stop() {
    timerTask?.cancel()
  }

  private func check() {
    userPIDDirectoryWritable = libkrbn_user_pid_directory_writable()

    var buffer = [Int8](repeating: 0, count: 32 * 1024)
    if libkrbn_configuration_monitor_get_parse_error_message(&buffer, buffer.count) {
      karabinerJSONParseErrorMessage = String(utf8String: buffer) ?? ""
    }

    let showAlert = !userPIDDirectoryWritable || !karabinerJSONParseErrorMessage.isEmpty

    // Display alerts only when the status changes.
    if previousShowAlert == nil || previousShowAlert != showAlert {
      previousShowAlert = showAlert

      ContentViewStates.shared.showDoctorAlert = showAlert
    }
  }
}
