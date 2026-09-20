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
        .modifier(StoredAppLanguage(pickerPlacement: .contentTop))
        .tabItem {
          AppLocalizedConstrainedLabel("multitouch_extension.tab.main", systemImage: "gearshape")
        }
        .tag(TabTag.main)

      SettingsPowerView()
        .modifier(StoredAppLanguage(pickerPlacement: .contentTop))
        .tabItem {
          AppLocalizedConstrainedLabel("multitouch_extension.tab.power", systemImage: "power")
        }
        .tag(TabTag.power)

      SettingsAdvancedView()
        .modifier(StoredAppLanguage(pickerPlacement: .contentTop))
        .tabItem {
          AppLocalizedConstrainedLabel("multitouch_extension.tab.advanced", systemImage: "hammer")
        }
        .tag(TabTag.advanced)

      SettingsActionView()
        .modifier(StoredAppLanguage(pickerPlacement: .contentTop))
        .tabItem {
          AppLocalizedConstrainedLabel(
            "multitouch_extension.tab.restart", systemImage: "arrow.clockwise")
        }
        .tag(TabTag.action)

      SettingsLogView()
        .modifier(StoredAppLanguage(pickerPlacement: .contentTop))
        .tabItem {
          AppLocalizedConstrainedLabel(
            "multitouch_extension.tab.log", systemImage: "list.bullet.rectangle")
        }
        .tag(TabTag.log)
    }
    .scenePadding()
    .frame(width: 600)
  }
}
