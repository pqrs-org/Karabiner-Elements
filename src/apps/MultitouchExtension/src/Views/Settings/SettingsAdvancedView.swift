import SwiftUI

struct SettingsAdvancedView: View {
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
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
        .padding(6.0)
      }

      Spacer()
    }.padding()
  }
}

struct SettingsAdvancedView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsAdvancedView()
  }
}
