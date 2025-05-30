import SwiftUI

enum NavigationTag: String {
  case main
  case advanced
  case action
}

struct SettingsView: View {
  @State private var selection: NavigationTag = .main

  var body: some View {
    VStack {
      HStack {
        VStack(alignment: .leading, spacing: 0) {
          Group {
            Button(
              action: {
                selection = .main
              },
              label: {
                SidebarLabelView(text: "Main", systemImage: "gearshape")
              }
            )
            .sidebarButtonStyle(selected: selection == .main)

            Button(
              action: {
                selection = .advanced
              },
              label: {
                SidebarLabelView(text: "Advanced", systemImage: "hammer")
              }
            )
            .sidebarButtonStyle(selected: selection == .advanced)
          }

          Divider()
            .padding(.vertical, 10.0)

          Group {
            Button(
              action: {
                selection = .action
              },
              label: {
                SidebarLabelView(text: "Restart", systemImage: "bolt.circle")
              }
            )
            .sidebarButtonStyle(selected: selection == .action)
          }

          Spacer()
        }
        .frame(width: 200)

        Divider()

        switch selection {
        case .main:
          SettingsMainView()
        case .advanced:
          SettingsAdvancedView()
        case .action:
          SettingsActionView()
        }
      }
    }.frame(width: 900, height: 550)
  }
}
