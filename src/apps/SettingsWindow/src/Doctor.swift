import Combine
import Foundation
import SwiftUI

final class Doctor: ObservableObject {
  static let shared = Doctor()

  @Published var userPIDDirectoryWritable: Bool

  private init() {
    userPIDDirectoryWritable = libkrbn_user_pid_directory_writable()

    if !userPIDDirectoryWritable {
      ContentViewStates.shared.showDoctorAlert = true
    } else {
      ContentViewStates.shared.showDoctorAlert = false
    }
  }
}
