import SwiftUI

struct VirtualKeyboardView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var systemPreferences = SystemPreferences.shared
  @ObservedObject private var grabberClient = LibKrbn.GrabberClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Keyboard type")) {
        // Use `ScrollView` instead of `List` to avoid `AttributeGraph: cycle detected through attribute` error.
        ScrollView {
          VStack(alignment: .leading, spacing: 6.0) {
            ForEach($systemPreferences.keyboardTypes) { $keyboardType in
              HStack {
                Picker(
                  "Country code: \(keyboardType.countryCode)", selection: $keyboardType.keyboardType
                ) {
                  Text("---").tag(-1)
                  Text("ANSI").tag(LibKrbn.KeyboardType.NamedType.ansi.rawValue)
                  Text("ISO").tag(LibKrbn.KeyboardType.NamedType.iso.rawValue)
                  Text("JIS").tag(LibKrbn.KeyboardType.NamedType.jis.rawValue)
                }.disabled(!grabberClient.enabled)
              }
            }
          }
          .padding(10)
          .background(Color(NSColor.textBackgroundColor))
        }
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

      GroupBox(label: Text("Sticky modifier keys")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $settings.virtualHIDKeyboardIndicateStickyModifierKeysState) {
              Text("Indicate sticky modifier keys state (Default: on)")
            }

            Spacer()
          }
        }
        .padding(6.0)
      }

      GroupBox(label: Text("Country code")) {
        VStack(alignment: .leading, spacing: 12.0) {
          VStack(alignment: .leading, spacing: 0) {
            Text("You usually don't need to change the country code value.")
            Text("")
            Text("This value is related to the keyboard layout (ANSI, ISO and JIS) on macOS.")
            Text(
              "macOS remembers the chosen keyboard layout with vendor id, product id and country code of the device."
            )
            Text(
              "If you are using other devices that has the same vendor id (0x16c0) and product id (0x27db), the keyboard layout is shared by the device and Karabiner-DriverKit-VirtualHIDKeyboard."
            )
            Text("In this case, you have to change the country code to avoid the conflict.")
          }

          HStack {
            Text("Country code:")

            IntTextField(
              value: $settings.virtualHIDKeyboardCountryCode,
              range: 0...255,
              step: 1,
              width: 50)

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
