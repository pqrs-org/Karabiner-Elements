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
      label: Text("Keyboard type:")
    ) {
      Text("ANSI (North America, most of Asia and others)").tag("ansi")
      Text("ISO (Europe, Latin America, Middle-East and others)").tag("iso")
      Text("JIS (Japanese)").tag("jis")
    }
    .pickerStyle(RadioGroupPickerStyle())
  }
}
