import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable, Hashable {
  case main
  case advanced
  case action

  var id: Self { self }

  var title: String {
    switch self {
    case .main: return "Main"
    case .advanced: return "Advanced"
    case .action: return "Restart"
    }
  }

  var systemImage: String {
    switch self {
    case .main: return "gearshape"
    case .advanced: return "hammer"
    case .action: return "bolt.circle"
    }
  }
}

struct SettingsView: View {
  @State private var selection: SidebarItem = .main

  var body: some View {
    NavigationSplitView(
      sidebar: {
        List(SidebarItem.allCases, selection: $selection) { item in
          Label(item.title, systemImage: item.systemImage)
            .padding(.vertical, 8)
        }
        .navigationSplitViewColumnWidth(200)
        .listStyle(.sidebar)
      },
      detail: {
        switch selection {
        case .main:
          SettingsMainView()
        case .advanced:
          SettingsAdvancedView()
        case .action:
          SettingsActionView()
        }
      }
    )
    .frame(
      minWidth: 900,
      maxWidth: .infinity,
      minHeight: 600,
      maxHeight: .infinity)
  }
}
