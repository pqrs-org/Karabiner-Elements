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

      GroupBox(label: Text("Karabiner Notification Window")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $settings.enableNotificationWindow) {
              Text("Enable Karabiner Notification Window (Default: on)")
            }
            .switchToggleStyle()

            Spacer()
          }

          HStack {
            Toggle(isOn: $settings.virtualHIDKeyboardIndicateStickyModifierKeysState) {
              Text("Indicate sticky modifier keys state (Default: on)")
            }
            .switchToggleStyle()

            Spacer()
          }

          VStack(alignment: .leading, spacing: 12.0) {
            Label(
              "What is the Karabiner Notification Window?",
              systemImage: "lightbulb"
            )
            VStack(alignment: .leading, spacing: 0.0) {
              Text(
                "Karabiner Notification Window is a window that displays messages, located at the bottom right of the screen. "
              )
              Text(
                "It is used for temporary alerts, displaying the status of sticky modifiers, and showing messages for some complex modifications."
              )
            }

            Image(decorative: "notification-window")
              .resizable()
              .aspectRatio(contentMode: .fit)
              .frame(height: 150)

          }
          .padding()
          .foregroundColor(Color.warningForeground)
          .background(Color.warningBackground)
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
