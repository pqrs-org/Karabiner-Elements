import SwiftUI

struct KeyboardTypeSelectorView: View {
  @ObservedObject private var settings = Settings.shared
  let onSelection: () -> Void

  init(onSelection: @escaping () -> Void = {}) {
    self.onSelection = onSelection
  }

  private var keyboardType: Binding<String> {
    Binding(
      get: {
        settings.configuration.selectedProfile.virtualHidKeyboard.keyboardTypeV2
      },
      set: { value in
        settings.configuration.selectedProfile.virtualHidKeyboard.keyboardTypeV2 = value
        onSelection()
      })
  }

  var body: some View {
    Picker(
      selection: keyboardType,
      label: AppLocalizedText("settings.setup.keyboard_type.label")
    ) {
      AppLocalizedText("settings.setup.keyboard_type.ansi").tag("ansi")
      AppLocalizedText("settings.setup.keyboard_type.iso").tag("iso")
      AppLocalizedText("settings.setup.keyboard_type.jis").tag("jis")
    }
    .pickerStyle(RadioGroupPickerStyle())
  }
}
