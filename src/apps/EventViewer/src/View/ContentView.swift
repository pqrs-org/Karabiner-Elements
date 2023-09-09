import SwiftUI

enum NavigationTag: String {
  case main
  case frontmostApplication
  case variables
  case devices
  case systemExtensions
  case unknownEvents
  case settings
}

struct ContentView: View {
  let window: NSWindow?

  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @State private var selection: NavigationTag = .main

  var body: some View {
    VStack {
      if inputMonitoringAlertData.showing {
        InputMonitoringAlertView(window: window)
      } else {
        HStack {
          VStack(alignment: .leading, spacing: 0) {
            Button(
              action: {
                selection = .main
              },
              label: {
                SidebarLabelView(text: "Main", systemImage: "magnifyingglass")
              }
            )
            .sidebarButtonStyle(selected: selection == .main)

            Button(
              action: {
                selection = .frontmostApplication
              },
              label: {
                SidebarLabelView(text: "Frontmost Application", systemImage: "triangle.circle")
              }
            )
            .sidebarButtonStyle(selected: selection == .frontmostApplication)

            Button(
              action: {
                selection = .variables
              },
              label: {
                SidebarLabelView(text: "Variables", systemImage: "cube")
              }
            )
            .sidebarButtonStyle(selected: selection == .variables)

            Button(
              action: {
                selection = .devices
              },
              label: {
                SidebarLabelView(text: "Devices", systemImage: "keyboard")
              }
            )
            .sidebarButtonStyle(selected: selection == .devices)

            Button(
              action: {
                selection = .systemExtensions
              },
              label: {
                SidebarLabelView(text: "System Extensions", systemImage: "puzzlepiece")
              }
            )
            .sidebarButtonStyle(selected: selection == .systemExtensions)

            Button(
              action: {
                selection = .unknownEvents
              },
              label: {
                SidebarLabelView(text: "Unknown Events", systemImage: "questionmark.square.dashed")
              }
            )
            .sidebarButtonStyle(selected: selection == .unknownEvents)

            Button(
              action: {
                selection = .settings
              },
              label: {
                SidebarLabelView(text: "Settings", systemImage: "gearshape")
              }
            )
            .sidebarButtonStyle(selected: selection == .settings)

            Spacer()
          }
          .frame(width: 250)

          Divider()

          switch selection {
          case .main:
            MainView()
          case .frontmostApplication:
            FrontmostApplicationView()
          case .variables:
            VariablesView()
          case .devices:
            DevicesView()
          case .systemExtensions:
            SystemExtensionsView()
          case .unknownEvents:
            UnknownEventsView()
          case .settings:
            SettingsView()
          }
        }
      }
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView(window: nil)
  }
}
