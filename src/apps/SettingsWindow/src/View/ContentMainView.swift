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
  case expert
  case action
  case log
  case changedSettings
  case systemExtensions
  case setup
  case debug

  var id: Self { self }

  var title: String {
    switch self {
    case .simpleModifications: return "settings.sidebar.item.simple_modifications"
    case .functionKeys: return "settings.sidebar.item.function_keys"
    case .complexModifications: return "settings.sidebar.item.complex_modifications"
    case .complexModificationsAdvanced: return "settings.sidebar.item.parameters"
    case .devices: return "settings.sidebar.item.devices"
    case .virtualKeyboard: return "settings.sidebar.item.virtual_keyboard"
    case .profiles: return "settings.sidebar.item.profiles"
    case .ui: return "settings.sidebar.item.ui"
    case .update: return "settings.sidebar.item.update"
    case .misc: return "settings.sidebar.item.misc"
    case .uninstall: return "settings.sidebar.item.uninstall"
    case .expert: return "settings.sidebar.item.expert"
    case .action: return "settings.sidebar.item.quit_restart"
    case .changedSettings: return "settings.sidebar.item.changed_settings"
    case .log: return "settings.sidebar.item.log"
    case .systemExtensions: return "settings.sidebar.item.system_extensions"
    case .setup: return "settings.sidebar.item.setup"
    case .debug: return "shared.debug.title"
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
    case .expert: return "flame"
    case .action: return "xmark.rectangle"
    case .changedSettings: return "slider.horizontal.3"
    case .log: return "list.bullet.rectangle"
    case .systemExtensions: return "puzzlepiece.extension"
    case .setup: return "checklist"
    case .debug: return "ladybug"
    }
  }
}

struct ContentMainView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var localization = AppLocalization.shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var systemPreferences = SystemPreferences.shared

  @State private var optionPressed = false
  @State private var selectedSidebarItem: SidebarItem = .simpleModifications

  struct SidebarSection {
    let title: String
    let items: [SidebarItem]
  }

  let sections: [SidebarSection] = [
    SidebarSection(
      title: "settings.sidebar.section.modifications",
      items: [
        .simpleModifications,
        .functionKeys,
        .complexModifications,
        .complexModificationsAdvanced,
      ]
    ),
    SidebarSection(
      title: "settings.sidebar.section.configurations",
      items: [
        .devices,
        .virtualKeyboard,
        .profiles,
        .ui,
      ]
    ),
    SidebarSection(
      title: "settings.sidebar.section.maintenance",
      items: [
        .update,
        .misc,
        .uninstall,
        .expert,
        .action,
      ]
    ),
    SidebarSection(
      title: "settings.sidebar.section.diagnostic",
      items: [
        .log,
        .changedSettings,
        .systemExtensions,
        .setup,
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
                sidebarRow(item)
              }
            } header: {
              AppLocalizedText(sections[section].title)
            }
          }
          if optionPressed || selectedSidebarItem == .debug {
            Section {
              sidebarRow(.debug)
            } header: {
              AppLocalizedText("shared.debug.section")
            }
          }
        }
        .onAppear {
          selectedSidebarItem = contentViewStates.navigationSelection
        }
        .onChange(of: selectedSidebarItem) { newValue in
          if contentViewStates.navigationSelection != newValue {
            contentViewStates.userSelectedNavigationItem(newValue)
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
          if settings.configuration.globalConfiguration.unsafeUi {
            Button(
              action: {
                selectedSidebarItem = .expert
              },
              label: {
                AppLocalizedLabel(
                  "settings.expert.unsafe_banner",
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
              AppLocalizedLabel(
                "settings.modifier_mappings.reset_hint",
                systemImage: WarningBorder.icon
              )

              OpenSystemSettingsButton(
                url: "x-apple.systempreferences:com.apple.preference.keyboard",
                label: {
                  AppLocalizedLabel(
                    "settings.system_settings.open",
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
              AppLocalizedLabel(
                "settings.save.failed",
                systemImage: ErrorBorder.icon,
                arguments: ["error": settings.saveErrorMessage]
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
          case .expert:
            ExpertView()
          case .action:
            ActionView()
          case .changedSettings:
            ChangedSettingsView()
          case .log:
            LogView()
          case .systemExtensions:
            SystemExtensionsView()
          case .setup:
            SetupView()
          case .debug:
            DebugAlertsView()
          }
        }
      }
    )
    .background(OptionKeyObserver(isPressed: $optionPressed).frame(width: 0, height: 0))
    .toolbar {
      ToolbarItem(placement: .primaryAction) {
        LanguagePicker(
          selection: $settings.configuration.globalConfiguration.uiLanguage,
          languages: AppLanguage.availableLanguages()
        )
        .fixedSize()
        .onAppear { resetUnavailableLanguage() }
        .onChange(of: settings.configurationLoaded) { _ in resetUnavailableLanguage() }
        .onChange(of: settings.configuration.globalConfiguration.uiLanguage) { _ in
          resetUnavailableLanguage()
        }
        .onChange(of: localization.catalog.languages) { _ in resetUnavailableLanguage() }
      }

    }
  }

  private func resetUnavailableLanguage() {
    // Wait for both settings and translations before validating the saved selection.
    guard settings.configurationLoaded, !localization.catalog.strings.isEmpty else { return }
    let selected = settings.configuration.globalConfiguration.uiLanguage
    if selected != "auto" && !AppLanguage.availableLanguages().contains(selected) {
      settings.configuration.globalConfiguration.uiLanguage = "auto"
    }
  }

  @ViewBuilder
  private func sidebarRow(_ item: SidebarItem) -> some View {
    HStack(spacing: 8.0) {
      Image(systemName: item.systemImage)
        .frame(width: 18.0)

      AppLocalizedText(item.title)
    }
    .padding(.vertical, 2.0)
    .tag(item)
  }
}
