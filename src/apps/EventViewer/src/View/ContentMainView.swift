import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable, Hashable {
  case inputEvents
  case rawInputEvents
  case rawInputRecords
  case frontmostApplication
  case variables
  case devices
  case settings

  var id: Self { self }

  var title: String {
    switch self {
    case .inputEvents: return "event_viewer.sidebar.input_events"
    case .rawInputEvents: return "event_viewer.sidebar.raw_input_events"
    case .rawInputRecords: return "event_viewer.sidebar.raw_input_records"
    case .frontmostApplication: return "event_viewer.sidebar.frontmost_application"
    case .variables: return "event_viewer.sidebar.variables"
    case .devices: return "event_viewer.sidebar.devices"
    case .settings: return "event_viewer.sidebar.settings"
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
          AppLocalizedLabel(item.title, systemImage: item.systemImage)
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
        case .settings:
          SettingsView()
        }
      }
    )
  }
}
