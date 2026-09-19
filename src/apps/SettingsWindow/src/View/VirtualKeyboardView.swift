import SwiftUI

struct VirtualKeyboardView: View {
  @ObservedObject private var settings = Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.virtual_keyboard.keyboard_type_v2")) {
          VStack(alignment: .leading, spacing: 6.0) {
            KeyboardTypeSelectorView()
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.virtual_keyboard.mouse_key")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              AppLocalizedText("settings.virtual_keyboard.mouse_key_xy_scale")

              IntTextField(
                value: $settings.configuration.selectedProfile.virtualHidKeyboard.mouseKeyXyScale,
                range: 0...100_000,
                step: 10,
                width: 50)

              Text("%")
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
