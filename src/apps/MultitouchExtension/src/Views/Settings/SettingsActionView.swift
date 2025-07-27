import SwiftUI

struct SettingsActionView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Action")) {
        VStack(alignment: .leading, spacing: 16) {
          Button(
            action: {
              // MultitouchExtension will be relaunched by launchd.
              NSApplication.shared.terminate(self)
            },
            label: {
              Label("Restart app", systemImage: "arrow.clockwise")
            })

          Label(
            "To disable the Multitouch Extension, configure it in the Misc tab of the Karabiner-Elements settings.",
            systemImage: "lightbulb"
          )
          .modifier(InfoBorder())
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }
}
