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
  @State private var selection: NavigationTag = NavigationTag.main

  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

  var body: some View {
    VStack {
      if inputMonitoringAlertData.showing {
        InputMonitoringAlertView(window: window)
      } else {
        HStack {
          VStack(alignment: .leading, spacing: 0) {
            Button(action: {
              selection = NavigationTag.main
            }) {
              SidebarLabelView(text: "Main", systemImage: "magnifyingglass")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.main)

            Button(action: {
              selection = NavigationTag.frontmostApplication
            }) {
              SidebarLabelView(text: "Frontmost Application", systemImage: "triangle.circle")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.frontmostApplication)

            Button(action: {
              selection = NavigationTag.variables
            }) {
              SidebarLabelView(text: "Variables", systemImage: "cube")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.variables)

            Button(action: {
              selection = NavigationTag.devices
            }) {
              SidebarLabelView(text: "Devices", systemImage: "keyboard")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.devices)

            Button(action: {
              selection = NavigationTag.systemExtensions
            }) {
              SidebarLabelView(text: "System Extensions", systemImage: "puzzlepiece")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.systemExtensions)

            Button(action: {
              selection = NavigationTag.unknownEvents
            }) {
              SidebarLabelView(text: "Unknown Events", systemImage: "questionmark.square.dashed")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.unknownEvents)

            Button(action: {
              selection = NavigationTag.settings
            }) {
              SidebarLabelView(text: "Settings", systemImage: "gearshape")
            }
            .sidebarButtonStyle(selected: selection == NavigationTag.settings)

            Spacer()
          }
          .frame(width: 250)

          Divider()

          switch selection {
          case NavigationTag.main:
            MainView()
          case NavigationTag.frontmostApplication:
            FrontmostApplicationView()
          case NavigationTag.variables:
            VariablesView()
          case NavigationTag.devices:
            DevicesView()
          case NavigationTag.systemExtensions:
            SystemExtensionsView()
          case NavigationTag.unknownEvents:
            UnknownEventsView()
          case NavigationTag.settings:
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
