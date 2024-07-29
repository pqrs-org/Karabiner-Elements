import Combine
import Foundation
import SwiftUI

final class Doctor: ObservableObject {
  static let shared = Doctor()

  private var timer: Timer?
  private var previousShowAlert: Bool?

  @Published var userPIDDirectoryWritable: Bool = true

  public func start() {
    timer = Timer.scheduledTimer(
      withTimeInterval: 3.0,
      repeats: true
    ) { [weak self] (_: Timer) in
      guard let self = self else { return }

      userPIDDirectoryWritable = libkrbn_user_pid_directory_writable()

      let showAlert = !userPIDDirectoryWritable

      // Display alerts only when the status changes.
      if previousShowAlert == nil || previousShowAlert != showAlert {
        previousShowAlert = showAlert

        ContentViewStates.shared.showDoctorAlert = showAlert
      }
    }

    timer?.fire()
  }

  public func stop() {
    timer?.invalidate()
    timer = nil
  }
}
