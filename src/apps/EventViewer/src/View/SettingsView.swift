import SwiftUI

struct SettingsView: View {
  @EnvironmentObject private var userSettings: UserSettings

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("Window behavior")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Toggle(isOn: $userSettings.forceStayTop) {
              Text("Force EventViewer to stay on top of other windows (Default: off)")
            }
            .switchToggleStyle()

            Spacer()
          }

          HStack {
            Toggle(isOn: $userSettings.showInAllSpaces) {
              Text("Show EventViewer in all spaces (Default: off)")
            }
            .switchToggleStyle()

            Spacer()
          }

          HStack {
            Toggle(isOn: $userSettings.quitUsingKeyboardShortcut) {
              Text("Enable Command+Q and Command+W shortcut (Default: off)")
            }
            .switchToggleStyle()

            Spacer()
          }
        }
        .padding()
      }

      Spacer()
    }
    .padding()
  }
}

struct SettingsView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsView()
  }
}
