import SwiftUI

struct KeyboardTypeSelectorView: View {
  @ObservedObject private var settings = Settings.shared

  var body: some View {
    Picker(
      selection: $settings.configuration.selectedProfile.virtualHidKeyboard.keyboardTypeV2,
      label: Text("Keyboard type:")
    ) {
      Text("ANSI (North America, most of Asia and others)").tag("ansi")
      Text("ISO (Europe, Latin America, Middle-East and others)").tag("iso")
      Text("JIS (Japanese)").tag("jis")
    }
    .pickerStyle(RadioGroupPickerStyle())
  }
}
