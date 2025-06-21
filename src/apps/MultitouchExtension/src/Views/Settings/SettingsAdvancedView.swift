import SwiftUI

struct SettingsAdvancedView: View {
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Sleep handling")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $userSettings.relaunchAfterWakeUpFromSleep) {
                Text("Relaunch after wake from sleep")
              }
              .switchToggleStyle()

              Text("(Default: on)")

              Spacer()
            }

            HStack {
              Text("Wait before relaunch:")

              IntTextField(
                value: $userSettings.relaunchWait,
                range: 0...10,
                step: 1,
                width: 40
              )
              .disabled(!userSettings.relaunchAfterWakeUpFromSleep)

              Text("seconds (Default: 3)")

              Spacer()
            }
          }
        }
        .padding()
      }

      GroupBox(label: Text("Delay")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              Text("Touch detection delay:")

              IntTextField(
                value: $userSettings.delayBeforeTurnOn,
                range: 0...10000,
                step: 100,
                width: 80)

              Text("milliseconds (Default: 0)")

              Spacer()
            }

            Text("(Increasing this value allows you to ignore unintended touch)")
          }

          VStack(alignment: .leading) {
            HStack {
              Text("Release detection delay:")

              IntTextField(
                value: $userSettings.delayBeforeTurnOff,
                range: 0...10000,
                step: 100,
                width: 80)

              Text("milliseconds (Default: 0)")

              Spacer()
            }

            Text("(Increasing this value allows you to ignore unintended release)")
          }
        }
        .padding()
      }

      GroupBox(label: Text("Palm Detection")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              Text("Threshold:")

              DoubleTextField(
                value: $userSettings.palmThreshold,
                range: 0...10,
                step: 0.5,
                maximumFractionDigits: 3,
                width: 80)

              Text("touch size threshold (Default: 2)")

              Spacer()
            }

            Text("(Increasing this value allows you to ignore unintended palm touches)")
          }

        }
        .padding()
      }
    }
  }
}
