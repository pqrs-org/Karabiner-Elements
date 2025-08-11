import SwiftUI

struct SettingsView: View {
  @EnvironmentObject private var userSettings: UserSettings

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("Window behavior")) {
        VStack(alignment: .leading, spacing: 12.0) {
          Toggle(isOn: $userSettings.forceStayTop) {
            Text("Force EventViewer to stay on top of other windows (Default: off)")
          }
          .switchToggleStyle()

          Toggle(isOn: $userSettings.showInAllSpaces) {
            Text("Show EventViewer in all spaces (Default: off)")
          }
          .switchToggleStyle()

          Toggle(isOn: $userSettings.quitUsingKeyboardShortcut) {
            Text("Enable Command+Q and Command+W shortcut (Default: off)")
          }
          .switchToggleStyle()
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    .padding()
    .frame(maxHeight: .infinity, alignment: .top)
  }
}
