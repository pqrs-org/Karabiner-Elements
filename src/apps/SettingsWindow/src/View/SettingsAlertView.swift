import SwiftUI

struct SettingsAlertView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 6.0) {
        VStack(alignment: .leading, spacing: 20.0) {
          AppLocalizedLabel(
            "settings.keyboard_type.select_prompt",
            systemImage: "gear"
          )
          .font(.system(size: 24))

          AppLocalizedText(
            "settings.keyboard_type.description"
          )

          VStack {
            KeyboardTypeSelectorView {
              contentViewStates.dismissCurrentAlert()
            }
          }
          .padding(20.0)
          .overlay(
            RoundedRectangle(cornerRadius: 8)
              .stroke(
                Color.accentColor,
                lineWidth: 3
              )
              .padding(2))

          AppLocalizedText(
            "settings.keyboard_type.change_later"
          )
        }
      }
      .padding()
      .frame(width: 650)

      SheetCloseButton {
        ContentViewStates.shared.dismissCurrentAlert()
      }
    }
    .onAppear {
      focus = true
      contentViewStates.navigationSelection = .virtualKeyboard
    }
  }
}
