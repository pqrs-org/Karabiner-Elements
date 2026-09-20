import SwiftUI

struct SetupAccessibilityView: View {
  private let accessibilityImage: String

  init(showsCloseButton: Bool = false) {
    accessibilityImage = "accessibility-macos26"
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        "settings.setup.accessibility.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      AppLocalizedText(
        "settings.setup.accessibility.description"
      )

      OpenSystemSettingsButton(
        url: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility",
        label: {
          AppLocalizedConstrainedLabel(
            "settings.setup.system_settings.open_privacy",
            systemImage: "arrow.forward.circle.fill")
        }
      )

      Image(decorative: accessibilityImage)
        .resizable()
        .scaledToFit()
        .border(Color.gray, width: 1)
    }
  }
}
