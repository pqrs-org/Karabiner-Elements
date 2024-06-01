import SwiftUI

struct SettingsMainView: View {
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Basic")) {
        VStack(alignment: .leading, spacing: 25.0) {
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

      GroupBox(label: Text("Area")) {
        VStack(alignment: .leading, spacing: 10.0) {
          HStack {
            IgnoredAreaView()

            Spacer()
          }

          Divider()

          HStack {
            FingerCountView()

            Spacer()
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
