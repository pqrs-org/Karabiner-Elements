import SwiftUI

enum TabTag: String {
  case main
  case power
  case advanced
  case action
  case log
}

struct SettingsView: View {
  @State private var selection: TabTag = .main

  var body: some View {
    TabView(selection: $selection) {
      SettingsMainView()
        .tabItem {
          Label("Main", systemImage: "gearshape")
        }
        .tag(TabTag.main)

      SettingsPowerView()
        .tabItem {
          Label("Power", systemImage: "power")
        }
        .tag(TabTag.power)

      SettingsAdvancedView()
        .tabItem {
          Label("Advanced", systemImage: "hammer")
        }
        .tag(TabTag.advanced)

      SettingsActionView()
        .tabItem {
          Label("Restart", systemImage: "arrow.clockwise")
        }
        .tag(TabTag.action)

      SettingsLogView()
        .tabItem {
          Label("Log", systemImage: "list.bullet.rectangle")
        }
        .tag(TabTag.log)
    }
    .scenePadding()
    .frame(width: 600)
  }
}
