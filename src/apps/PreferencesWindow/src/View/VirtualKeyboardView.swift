import SwiftUI

struct VirtualKeyboardView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var systemPreferences = SystemPreferences.shared
  @ObservedObject private var grabberClient = LibKrbn.GrabberClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Keyboard type")) {
        VStack(alignment: .leading, spacing: 6.0) {
          HStack {
            if !systemPreferences.keyboardTypeChanged {
              VStack {
                Label(
                  "Log out will be required when you changed keyboard type (ANSI, ISO or JIS)",
                  systemImage: "lightbulb"
                )
                .padding()
              }
              .foregroundColor(Color.warningForeground)
              .background(Color.warningBackground)
            } else {
              VStack {
                Label(
                  "Log out is required to apply keyboard type changes",
                  systemImage: "lightbulb"
                )
                .padding()
              }
              .foregroundColor(Color.errorForeground)
              .background(Color.errorBackground)
            }

            Spacer()

            VStack {
              Text("Selection will be reflected immediately")
            }
            .padding()
            .foregroundColor(Color.infoForeground)
            .background(Color.infoBackground)
          }

          ForEach($systemPreferences.keyboardTypes) { $keyboardType in
            HStack {
              Picker(
                "Country code: \(keyboardType.countryCode)",
                selection: $keyboardType.keyboardType
              ) {
                if keyboardType.keyboardType < 0 {
                  Text("---").tag(-1)
                }
                Text("ANSI").tag(LibKrbn.KeyboardType.NamedType.ansi.rawValue)
                Text("ISO").tag(LibKrbn.KeyboardType.NamedType.iso.rawValue)
                Text("JIS").tag(LibKrbn.KeyboardType.NamedType.jis.rawValue)
              }.disabled(!grabberClient.enabled)

              Spacer()

              if settings.virtualHIDKeyboardCountryCode == keyboardType.countryCode {
                Label("Selected", systemImage: "checkmark.square.fill")
              } else {
                Button(action: {
                  settings.virtualHIDKeyboardCountryCode = keyboardType.countryCode
                }) {
                  Label("Select", systemImage: "square")
                }
              }
            }
          }
        }
        .padding(6.0)
        .background(Color(NSColor.textBackgroundColor))
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
