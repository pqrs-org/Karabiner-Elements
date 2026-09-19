import SwiftUI

struct SettingsActionView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: AppLocalizedText("multitouch_extension.action.title")) {
        VStack(alignment: .leading, spacing: 16) {
          Button(
            action: {
              // MultitouchExtension will be relaunched by launchd.
              NSApplication.shared.terminate(self)
            },
            label: {
              AppLocalizedLabel(
                "multitouch_extension.action.restart", systemImage: "arrow.clockwise")
            })

          AppLocalizedLabel(
            "multitouch_extension.action.disable_hint",
            systemImage: InfoBorder.icon
          )
          .modifier(InfoBorder())
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }
}
