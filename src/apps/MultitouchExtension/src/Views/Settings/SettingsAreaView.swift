import SwiftUI

struct SettingsAreaView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
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

struct SettingsAreaView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsAreaView()
  }
}
