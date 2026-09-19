import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable, Hashable {
  case inputEvents
  case rawInputEvents
  case rawInputRecords
  case frontmostApplication
  case variables
  case devices
  case settings
  case debug

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
    case .debug: return "settings.debug.title"
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
    case .debug: return "ladybug"
    }
  }
}

struct ContentMainView: View {
  @State private var optionPressed = false
  @State private var selection: SidebarItem = .inputEvents

  var body: some View {
    NavigationSplitView(
      sidebar: {
        List(selection: $selection) {
          ForEach(SidebarItem.allCases.filter { $0 != .debug }) { item in
            sidebarRow(item)
          }
          if optionPressed || selection == .debug {
            Section {
              sidebarRow(.debug)
            } header: {
              AppLocalizedText("settings.debug.section")
            }
          }
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
        case .debug:
          DebugAlertsView()
        }
      }
    )
    .background(OptionKeyObserver(isPressed: $optionPressed).frame(width: 0, height: 0))
  }

  private func sidebarRow(_ item: SidebarItem) -> some View {
    AppLocalizedLabel(item.title, systemImage: item.systemImage)
      .padding(.vertical, 8)
      .tag(item)
  }
}
