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
  @State private var layoutResetRequest = UUID()
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
              SidebarLayoutResetButton(request: $layoutResetRequest)
            } header: {
              AppLocalizedText("settings.debug.section")
            }
          }
        }
        .modifier(
          SidebarStyle(
            resetRequest: layoutResetRequest,
            defaultContentSize: ContentView.defaultContentSize
          )
        )
      },
      detail: {
        Group {
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
        // Keep fixed-size localized text from inflating the detail pane's minimum height.
        .frame(minHeight: 0, maxHeight: .infinity, alignment: .topLeading)
      }
    )
    .background(OptionKeyObserver(isPressed: $optionPressed).frame(width: 0, height: 0))
  }

  private func sidebarRow(_ item: SidebarItem) -> some View {
    SidebarLabel(title: item.title, systemImage: item.systemImage)
      .tag(item)
  }
}
