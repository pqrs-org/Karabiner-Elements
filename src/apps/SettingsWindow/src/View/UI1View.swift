import SwiftUI

struct UI1View: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Menu bar")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $settings.showIconInMenuBar) {
              Text("Show icon in menu bar (Default: on)")
            }
            .switchToggleStyle()

            Spacer()
          }

          HStack {
            Toggle(isOn: $settings.showProfileNameInMenuBar) {
              Text("Show profile name in menu bar (Default: off)")
            }
            .switchToggleStyle()

            Spacer()
          }

          HStack {
            Toggle(isOn: $settings.askForConfirmationBeforeQuitting) {
              Text("Ask for confirmation when quitting (Default: on)")
            }
            .switchToggleStyle()

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
            .switchToggleStyle()

            Spacer()
          }
        }
        .padding(6.0)
      }

      GroupBox(label: Text("System")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $settings.enableNotifications) {
              Text("Enable notifications (Default: on)")
            }
            .switchToggleStyle()

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

struct UI1View_Previews: PreviewProvider {
  static var previews: some View {
    UI1View()
  }
}
