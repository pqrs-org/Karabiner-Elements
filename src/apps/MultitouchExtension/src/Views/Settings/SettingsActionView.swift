import SwiftUI

struct SettingsActionView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Action")) {
        VStack(alignment: .leading, spacing: 16) {
          HStack {
            Button(
              action: {
                // MultitouchExtension will be relaunched by launchd.
                NSApplication.shared.terminate(self)
              },
              label: {
                Label("Restart app", systemImage: "arrow.clockwise")
              })

            Spacer()
          }

          VStack {
            Text(
              "To disable the Multitouch Extension, configure it in the Misc tab of the Karabiner-Elements settings."
            )
          }
          .padding()
          .foregroundColor(Color.infoForeground)
          .background(Color.infoBackground)
        }.padding()
      }
    }
  }
}
