import SwiftUI

struct SetupAccessibilityView: View {
  private let accessibilityImage: String

  init(showsCloseButton: Bool = false) {
    accessibilityImage = "accessibility-macos26"
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      Label(
        "Please allow access to accessibility features",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      Text(
        """
        Karabiner-Core-Service uses accessibility features.
        Please allow it on Privacy & Security System Settings.
        """
      )
      .fixedSize(horizontal: false, vertical: true)

      OpenSystemSettingsButton(
        url: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility",
        label: {
          Label(
            "Open Privacy & Security System Settings...",
            systemImage: "arrow.forward.circle.fill")
        }
      )

      Image(decorative: accessibilityImage)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .border(Color.gray, width: 1)
    }
  }
}
