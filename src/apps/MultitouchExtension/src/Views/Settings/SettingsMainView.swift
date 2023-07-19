import SwiftUI

struct SettingsMainView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      HStack {
        Text("SettingsMainView")

        Spacer()
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
