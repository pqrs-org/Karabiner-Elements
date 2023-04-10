import SwiftUI

struct UIView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("Icon")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Image(decorative: "000-KarabinerElements")
              .resizable()
              .frame(width: 64.0, height: 64.0)

            Image(decorative: "000-EventViewer")
              .resizable()
              .frame(width: 64.0, height: 64.0)

            Image(decorative: "000-MultitouchExtension")
              .resizable()
              .frame(width: 64.0, height: 64.0)

            Spacer()
          }

          Divider()

          HStack {
            Image(decorative: "001-KarabinerElements")
              .resizable()
              .frame(width: 64.0, height: 64.0)

            Image(decorative: "001-EventViewer")
              .resizable()
              .frame(width: 64.0, height: 64.0)

            Image(decorative: "001-MultitouchExtension")
              .resizable()
              .frame(width: 64.0, height: 64.0)

            Spacer()
          }
        }
        .padding(6.0)
      }

      Spacer()
    }
    .padding()
  }
}

struct UIView_Previews: PreviewProvider {
  static var previews: some View {
    UIView()
  }
}
