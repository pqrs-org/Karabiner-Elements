import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable, Hashable {
  case inputEvents
  case rawInputEvents
  case rawInputRecords
  case frontmostApplication
  case variables
  case devices
  case unknownEvents
  case settings

  var id: Self { self }

  var title: String {
    switch self {
    case .inputEvents: return "Capture Input Events"
    case .rawInputEvents: return "Capture Raw Input Events"
    case .rawInputRecords: return "Capture Raw Input Records"
    case .frontmostApplication: return "Frontmost Application"
    case .variables: return "Variables"
    case .devices: return "Devices"
    case .unknownEvents: return "Unknown Events"
    case .settings: return "Settings"
    }
  }

  var systemImage: String {
    switch self {
    case .inputEvents: return "magnifyingglass"
    case .rawInputEvents: return "keyboard.badge.ellipsis"
    case .rawInputRecords: return "waveform.path.ecg"
    case .frontmostApplication: return "triangle.circle"
    case .variables: return "cube"
    case .devices: return "keyboard"
    case .unknownEvents: return "questionmark.square.dashed"
    case .settings: return "gear"
    }
  }
}

struct ContentMainView: View {
  @State private var selection: SidebarItem = .inputEvents

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
        case .inputEvents:
          CaptureInputEventsView()
        case .rawInputEvents:
          CaptureRawInputEventsView()
        case .rawInputRecords:
          CaptureRawInputRecordsView()
        case .frontmostApplication:
          FrontmostApplicationView()
        case .variables:
          VariablesView()
        case .devices:
          DevicesView()
        case .unknownEvents:
          UnknownEventsView()
        case .settings:
          SettingsView()
        }
      }
    )
  }
}
