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
            Spacer()

            if !systemPreferences.keyboardTypeChanged {
              VStack {
                Label(
                  "Log out will be required when you changed keyboard type (ANSI, ISO or JIS) from the drop-down list",
                  systemImage: "lightbulb"
                )
              }
              .padding()
              .foregroundColor(Color.warningForeground)
              .background(Color.warningBackground)
            } else {
              VStack {
                Label(
                  "Log out is required to apply keyboard type changes",
                  systemImage: "lightbulb"
                )
              }
              .padding()
              .foregroundColor(Color.errorForeground)
              .background(Color.errorBackground)
            }
          }

          // Use `ScrollView` instead of `List` to avoid `AttributeGraph: cycle detected through attribute` error.
          ScrollView {
            ForEach($systemPreferences.keyboardTypes) { $keyboardType in
              HStack {
                Button(action: {
                  settings.virtualHIDKeyboardCountryCode = keyboardType.countryCode
                }) {
                  HStack {
                    HStack {
                      if settings.virtualHIDKeyboardCountryCode == keyboardType.countryCode {
                        Image(systemName: "circle.circle.fill")
                      } else {
                        Image(systemName: "circle")
                      }
                    }
                    .foregroundColor(.accentColor)

                    Text("Country code: \(keyboardType.countryCode)")
                  }
                }
                .buttonStyle(.plain)

                Picker("", selection: $keyboardType.keyboardType) {
                  if keyboardType.keyboardType < 0 {
                    Text("---").tag(-1)
                  }
                  Text("ANSI (North America, most of Asia and others)").tag(
                    LibKrbn.KeyboardType.NamedType.ansi.rawValue)
                  Text("ISO (Europe, Latin America, Middle-East and others)").tag(
                    LibKrbn.KeyboardType.NamedType.iso.rawValue)
                  Text("JIS (Japanese)").tag(LibKrbn.KeyboardType.NamedType.jis.rawValue)
                }.disabled(!grabberClient.enabled)

                Spacer()
              }
            }
          }

          HStack {
            VStack(alignment: .leading, spacing: 0.0) {
              Text("Note:")
              Text(
                "The keyboard type configurations (ANSI, ISO, JIS) are shared by all of this Mac users."
              )
              Text("The country code selection is saved for each user.")
            }

            Spacer()
          }
          .padding()
          .foregroundColor(Color.warningForeground)
          .background(Color.warningBackground)
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
