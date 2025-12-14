import SwiftUI

struct SettingsPowerView: View {
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Power")) {
        VStack(alignment: .leading, spacing: 10.0) {
          HStack {
            Toggle(isOn: $userSettings.allowUserInteractiveActivity) {
              Text(
                "Enable user-interactive activity for better responsiveness to user input"
              )
            }
            .switchToggleStyle()

            Text("(Default: off)")
          }

          Text(
            "(This setting consumes battery power and may prevent the system from sleeping)",
          )
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
      }

      GroupBox(label: Text("User-interactive activity options")) {
        VStack(alignment: .leading, spacing: 10.0) {
          HStack {
            Toggle(isOn: $userSettings.keepUserInteractiveActivityDuringDisplaySleep) {
              Text("Keep user-interactive activity running while the display sleeps")
            }
            .switchToggleStyle()

            Text("(Default: off)")
          }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
      }
      .disabled(!userSettings.allowUserInteractiveActivity)
    }
  }
}
