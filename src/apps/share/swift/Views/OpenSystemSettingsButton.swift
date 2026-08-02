import AppKit
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
          let applications = NSRunningApplication.runningApplications(
            withBundleIdentifier: "com.apple.systempreferences"
          )

          for application in applications {
            application.terminate()
          }

          for _ in 0..<5 {
            if applications.allSatisfy(\.isTerminated) {
              break
            }

            try? await Task.sleep(for: .milliseconds(500))
          }

          // Since Launch Services may still hold onto entries right after the process ends, we wait briefly.
          try? await Task.sleep(for: .milliseconds(500))

          if let u = URL(string: url) {
            NSWorkspace.shared.open(u)
          }

          isProcessing = false
        }
      },
      label: {
        ZStack {
          label()
            .opacity(isProcessing ? 0 : 1)

          if isProcessing {
            ProgressView()
              .controlSize(.small)
              .accessibilityLabel("Opening System Settings")
          }
        }
      }
    )
    .disabled(isProcessing)
  }
}
