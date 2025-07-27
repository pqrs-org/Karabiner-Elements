import SwiftUI

enum SidebarItem: String, CaseIterable, Identifiable, Hashable {
  case simpleModifications
  case functionKeys
  case complexModifications
  case complexModificationsAdvanced
  case devices
  case virtualKeyboard
  case profiles
  case ui
  case update
  case misc
  case uninstall
  case log
  case expert
  case action

  var id: Self { self }

  var title: String {
    switch self {
    case .simpleModifications: return "Simple Modifications"
    case .functionKeys: return "Function Keys"
    case .complexModifications: return "Complex Modifications"
    case .complexModificationsAdvanced: return "Parameters"
    case .devices: return "Devices"
    case .virtualKeyboard: return "Virtual Keyboard"
    case .profiles: return "Profiles"
    case .ui: return "UI"
    case .update: return "Update"
    case .misc: return "Misc"
    case .uninstall: return "Uninstall"
    case .log: return "Log"
    case .expert: return "Expert"
    case .action: return "Quit, Restart"
    }
  }

  var systemImage: String {
    switch self {
    case .simpleModifications: return "gearshape"
    case .functionKeys: return "speaker.wave.2.circle"
    case .complexModifications: return "gearshape.2"
    case .complexModificationsAdvanced: return "dial.min"
    case .devices: return "keyboard"
    case .virtualKeyboard: return "puzzlepiece"
    case .profiles: return "person.3"
    case .ui: return "switch.2"
    case .update: return "network"
    case .misc: return "leaf"
    case .uninstall: return "trash"
    case .log: return "doc.plaintext"
    case .expert: return "flame"
    case .action: return "bolt.circle"
    }
  }
}

struct ContentMainView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var systemPreferences = SystemPreferences.shared

  @State private var selectedSidebarItem: SidebarItem = .simpleModifications

  private let padding = 6.0

  struct SidebarSection {
    let title: String
    let items: [SidebarItem]
  }

  let sections: [SidebarSection] = [
    SidebarSection(
      title: "Modifications",
      items: [
        .simpleModifications,
        .functionKeys,
        .complexModifications,
        .complexModificationsAdvanced,
      ]
    ),
    SidebarSection(
      title: "Configurations",
      items: [
        .devices,
        .virtualKeyboard,
        .profiles,
        .ui,
      ]
    ),
    SidebarSection(
      title: "Maintenance",
      items: [
        .update,
        .misc,
        .uninstall,
      ]
    ),
    SidebarSection(
      title: "Tools",
      items: [
        .log,
        .expert,
        .action,
      ]
    ),
  ]

  var body: some View {
    NavigationSplitView(
      sidebar: {
        List(selection: $selectedSidebarItem) {
          ForEach(sections.indices, id: \.self) { section in
            Section {
              ForEach(sections[section].items) { item in
                Label(item.title, systemImage: item.systemImage)
                  .padding(.vertical, 4.0)
              }
            } header: {
              Text(sections[section].title)
            }
          }
        }
        .onAppear {
          selectedSidebarItem = contentViewStates.navigationSelection
        }
        .onChange(of: selectedSidebarItem) { newValue in
          if contentViewStates.navigationSelection != newValue {
            contentViewStates.navigationSelection = newValue
          }
        }
        .onChange(of: contentViewStates.navigationSelection) { newValue in
          if selectedSidebarItem != newValue {
            selectedSidebarItem = newValue
          }
        }
        .navigationSplitViewColumnWidth(250)
        .listStyle(.sidebar)
      },
      detail: {
        VStack(alignment: .leading, spacing: 0) {
          if settings.unsafeUI {
            Button(
              action: {
                selectedSidebarItem = .expert
              },
              label: {
                Label(
                  "The unsafe configuration is enabled, so the foolproof feature is currently inactive.",
                  systemImage: "exclamationmark.triangle"
                )
              }
            )
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(8.0)
            .buttonStyle(PlainButtonStyle())
            .background(Color.red)
            .foregroundColor(.white)
          }

          if systemPreferences.virtualHIDKeyboardModifierMappingsExists {
            VStack(alignment: .leading) {
              Label(
                "macOS also remaps modifier keys. It's recommended to restore defaults and configure them via Karabiner-Elements.\n"
                  + "\n"
                  + "You can reset the macOS setting by following steps:\n"
                  + "1. Open System Settings and go to Keyboard Shortcuts... > Modifier Keys.\n"
                  + "2. Choose Karabiner DriverKit VirtualHIDKeyboard.\n"
                  + "3. Click the Restore Defaults button.",
                systemImage: WarningBorder.icon
              )

              Button(
                action: {
                  if let url = URL(
                    string: "x-apple.systempreferences:com.apple.preference.keyboard"
                  ) {
                    NSWorkspace.shared.open(url)
                  }
                },
                label: {
                  Label(
                    "Open System Settings...",
                    systemImage: "arrow.up.forward.app"
                  )
                }
              )
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .modifier(WarningBorder())
            .padding()
          }

          if settings.saveErrorMessage != "" {
            VStack(alignment: .leading) {
              Label(
                "Save failed:\n\(settings.saveErrorMessage)",
                systemImage: "exclamationmark.circle.fill"
              )
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .modifier(ErrorBorder())
            .padding()
          }

          switch selectedSidebarItem {
          case .simpleModifications:
            SimpleModificationsView()
          case .functionKeys:
            FunctionKeysView()
          case .complexModifications:
            ComplexModificationsView()
          case .complexModificationsAdvanced:
            ComplexModificationsAdvancedView()
          case .devices:
            DevicesView()
          case .virtualKeyboard:
            VirtualKeyboardView()
          case .profiles:
            ProfilesView()
          case .ui:
            UIView()
          case .update:
            UpdateView()
          case .misc:
            MiscView()
          case .uninstall:
            UninstallView()
          case .log:
            LogView()
          case .expert:
            ExpertView()
          case .action:
            ActionView()
          }
        }
      }
    )
  }
}
