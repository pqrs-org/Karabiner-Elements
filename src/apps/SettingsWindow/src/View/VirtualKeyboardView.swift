import SwiftUI

struct VirtualKeyboardView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var systemPreferences = SystemPreferences.shared
  @ObservedObject private var grabberClient = LibKrbn.GrabberClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Keyboard type")) {
        VStack(alignment: .leading, spacing: 6.0) {
          Picker(
            selection: $settings.virtualHIDKeyboardKeyboardTypeV2, label: Text("Keyboard type:")
          ) {
            Text("ANSI (North America, most of Asia and others)").tag("ansi")
            Text("ISO (Europe, Latin America, Middle-East and others)").tag("iso")
            Text("JIS (Japanese)").tag("jis")
          }
          .pickerStyle(RadioGroupPickerStyle())
          .disabled(!grabberClient.connected)
        }
        .padding(6.0)
      }

      GroupBox(label: Text("Mouse key")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Text("Tracking speed:")

            IntTextField(
              value: $settings.virtualHIDKeyboardMouseKeyXYScale,
              range: 0...100_000,
              step: 10,
              width: 50)

            Text("%")

            Spacer()
          }
        }
        .padding(6.0)
      }

      Spacer()
    }
    .padding()
  }
}

struct VirtualKeyboardView_Previews: PreviewProvider {
  static var previews: some View {
    VirtualKeyboardView()
  }
}
