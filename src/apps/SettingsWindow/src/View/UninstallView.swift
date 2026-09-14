import SwiftUI

struct UninstallView: View {
  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.uninstall.title")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Button(
              role: .destructive,
              action: {
                krbn_launch_uninstaller()

                NSApplication.shared.terminate(nil)
              },
              label: {
                AppLocalizedLabel("settings.uninstall.launch", systemImage: "trash")
                  .buttonLabelStyle()
              }
            )
            .deleteButtonStyle()
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
