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

  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

  var body: some View {
    VStack {
      if inputMonitoringAlertData.showing {
        InputMonitoringAlertView(window: window)
      } else {
        HStack {
          VStack(alignment: .leading, spacing: 0) {
            Button(action: {
              selection = .main
            }) {
              SidebarLabelView(text: "Main", systemImage: "magnifyingglass")
            }
            .sidebarButtonStyle(selected: selection == .main)

            Button(action: {
              selection = .frontmostApplication
            }) {
              SidebarLabelView(text: "Frontmost Application", systemImage: "triangle.circle")
            }
            .sidebarButtonStyle(selected: selection == .frontmostApplication)

            Button(action: {
              selection = .variables
            }) {
              SidebarLabelView(text: "Variables", systemImage: "cube")
            }
            .sidebarButtonStyle(selected: selection == .variables)

            Button(action: {
              selection = .devices
            }) {
              SidebarLabelView(text: "Devices", systemImage: "keyboard")
            }
            .sidebarButtonStyle(selected: selection == .devices)

            Button(action: {
              selection = .systemExtensions
            }) {
              SidebarLabelView(text: "System Extensions", systemImage: "puzzlepiece")
            }
            .sidebarButtonStyle(selected: selection == .systemExtensions)

            Button(action: {
              selection = .unknownEvents
            }) {
              SidebarLabelView(text: "Unknown Events", systemImage: "questionmark.square.dashed")
            }
            .sidebarButtonStyle(selected: selection == .unknownEvents)

            Button(action: {
              selection = .settings
            }) {
              SidebarLabelView(text: "Settings", systemImage: "gearshape")
            }
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
