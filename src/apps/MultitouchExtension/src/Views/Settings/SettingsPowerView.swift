import SwiftUI

struct SettingsPowerView: View {
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Power")) {
        VStack(alignment: .leading, spacing: 10.0) {
          HStack {
            Toggle(isOn: $userSettings.allowUserInteractiveActivity) {
              Text("Enhance responsiveness to user interaction")
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

      if userSettings.allowUserInteractiveActivity {
        GroupBox(label: Text("User interaction")) {
          VStack(alignment: .leading, spacing: 10.0) {
            HStack {
              Toggle(isOn: $userSettings.stopUserInteractiveActivityOnDisplaySleep) {
                Text("Stop the user interactive activity on display sleep")
              }
              .switchToggleStyle()

              Text("(Default: on)")
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
          .padding()
        }
      }
    }
  }
}
