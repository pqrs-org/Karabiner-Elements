import SwiftUI

struct UninstallView: View {
  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Uninstall")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Button(
              role: .destructive,
              action: {
                libkrbn_launch_uninstaller()

                NSApplication.shared.terminate(nil)
              },
              label: {
                Label("Launch uninstaller", systemImage: "trash")
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
