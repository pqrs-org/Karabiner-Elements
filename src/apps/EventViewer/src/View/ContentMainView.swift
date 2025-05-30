import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable, Hashable {
  case main
  case frontmostApplication
  case variables
  case devices
  case systemExtensions
  case unknownEvents
  case settings

  var id: Self { self }

  var title: String {
    switch self {
    case .main: return "Main"
    case .frontmostApplication: return "Frontmost Application"
    case .variables: return "Variables"
    case .devices: return "Devices"
    case .systemExtensions: return "System Extensions"
    case .unknownEvents: return "Unknown Events"
    case .settings: return "Settings"
    }
  }

  var systemImage: String {
    switch self {
    case .main: return "magnifyingglass"
    case .frontmostApplication: return "triangle.circle"
    case .variables: return "cube"
    case .devices: return "keyboard"
    case .systemExtensions: return "puzzlepiece"
    case .unknownEvents: return "questionmark.square.dashed"
    case .settings: return "gearshape"
    }
  }
}

struct ContentMainView: View {
  @State private var selection: SidebarItem = .main

  var body: some View {
    NavigationSplitView(
      sidebar: {
        List(SidebarItem.allCases, selection: $selection) { item in
          Label(item.title, systemImage: item.systemImage)
            .padding(.vertical, 8)
        }
        .navigationSplitViewColumnWidth(250)
        .listStyle(.sidebar)
      },
      detail: {
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
    )
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
  }
}
