import SwiftUI

struct SettingsMainView: View {
  @ObservedObject private var userSettings = UserSettings.shared
  @ObservedObject private var fingerManager = FingerManager.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Basic")) {
        VStack(alignment: .leading, spacing: 25.0) {
          HStack {
            Toggle(isOn: $userSettings.openAtLogin) {
              Text("Open at login")
            }
            .switchToggleStyle()

            Text("(Default: off)")

            Spacer()
          }

          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $userSettings.hideIconInDock) {
                Text("Hide icon in Dock")
              }
              .switchToggleStyle()

              Text("(Default: off)")

              Spacer()
            }

            Text("(You need to restart app to enable/disable this option)")
          }
        }
        .padding(6.0)
      }

      Spacer()
    }.padding()
  }
}

struct SettingsMainView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsMainView()
  }
}
