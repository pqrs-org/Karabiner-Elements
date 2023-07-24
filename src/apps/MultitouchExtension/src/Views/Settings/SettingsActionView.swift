import SwiftUI

struct SettingsActionView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Action")) {
        VStack(alignment: .leading, spacing: 16) {
          HStack {
            Button(action: {
              Relauncher.relaunch()
            }) {
              Label("Restart app", systemImage: "arrow.clockwise")
            }

            Spacer()
          }

          HStack {
            Button(action: {
              NSApplication.shared.terminate(self)
            }) {
              Label("Quit app", systemImage: "xmark.circle.fill")
            }

            Spacer()
          }
        }.padding()
      }

      Spacer()
    }.padding()
  }
}

struct SettingsActionView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsActionView()
      .previewLayout(.sizeThatFits)
  }
}
