import SwiftUI

enum NavigationTag: String {
  case main
  case advanced
}

struct SettingsView: View {
  @State private var selection: NavigationTag = .main

  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

  var body: some View {
    VStack {
      HStack {
        VStack(alignment: .leading, spacing: 0) {
          Button(action: {
            selection = .main
          }) {
            SidebarLabelView(text: "Main", systemImage: "gearshape")
          }
          .sidebarButtonStyle(selected: selection == .main)

          Button(action: {
            selection = .advanced
          }) {
            SidebarLabelView(text: "Advanced", systemImage: "hammer")
          }
          .sidebarButtonStyle(selected: selection == .advanced)

          Spacer()
        }
        .frame(width: 250)

        Divider()

        switch selection {
        case .main:
          SettingsMainView()
        case .advanced:
          SettingsAdvancedView()
        }
      }
    }.frame(width: 900, height: 550)
  }
}

struct SettingsView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsView()
      .previewLayout(.sizeThatFits)
  }
}
