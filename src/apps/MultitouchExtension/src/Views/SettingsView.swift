import SwiftUI

enum TabTag: String {
  case main
  case advanced
  case action
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

      SettingsAdvancedView()
        .tabItem {
          Label("Advanced", systemImage: "hammer")
        }
        .tag(TabTag.advanced)

      SettingsActionView()
        .tabItem {
          Label("Restart", systemImage: "bolt.circle")
        }
        .tag(TabTag.action)
    }
    .scenePadding()
    .frame(width: 600)
  }
}
