import SwiftUI

struct OpenSystemSettingsButton<LabelView: View>: View {
  let url: String
  let label: () -> LabelView

  @State private var isProcessing = false

  var body: some View {
    Button(
      action: {
        guard !isProcessing else { return }

        isProcessing = true

        Task {
          libkrbn_killall_system_settings()

          if let u = URL(string: url) {
            NSWorkspace.shared.open(u)
          }

          await MainActor.run {
            isProcessing = false
          }
        }
      },
      label: label
    )
    .disabled(isProcessing)
  }
}
