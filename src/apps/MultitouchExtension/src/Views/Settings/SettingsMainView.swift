import SwiftUI

struct SettingsMainView: View {
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Area")) {
        VStack(alignment: .center, spacing: 10.0) {
          IgnoredAreaView()

          Divider()

          FingerCountView()
        }
        .frame(maxWidth: .infinity, alignment: .center)
        .padding()
      }
    }
  }
}
