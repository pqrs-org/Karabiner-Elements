import SwiftUI

struct AccessibilityAlertView: View {
  @FocusState var focus: Bool

  private let accessibilityImage: String

  init() {
    accessibilityImage = "accessibility-macos26"
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(spacing: 20.0) {
        Label(
          "Please allow access to accessibility features",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        Text(
          "Karabiner-Core-Service uses accessibility features.\nPlease allow it on Privacy & Security System Settings."
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
        .focused($focus)

        Image(decorative: accessibilityImage)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .border(Color.gray, width: 1)
          .frame(height: 300)
      }
      .padding()
      .frame(width: 700)

      SheetCloseButton {
        ContentViewStates.shared.dismissCurrentAlert()
      }
    }
    .onAppear {
      focus = true
    }
  }
}
