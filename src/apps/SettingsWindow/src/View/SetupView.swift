import SwiftUI

enum SetupItem: String, CaseIterable, Identifiable, Hashable {
  case services
  case accessibility
  case inputMonitoring
  case driverExtension

  var id: Self { self }

  var title: String {
    switch self {
    case .services: return "settings.setup.item.services"
    case .accessibility: return "settings.setup.item.accessibility"
    // Depending on the macOS version, granting Accessibility permission may also allow Input Monitoring.
    // In that case, both requirements are considered completed once Accessibility is granted, so if we call it
    // "Input Monitoring", users may wonder why it is marked as completed even though they did not explicitly
    // allow Input Monitoring.
    // To avoid that confusion, the displayed label is changed to "Capture Input Events".
    case .inputMonitoring: return "settings.setup.item.input_monitoring"
    case .driverExtension: return "settings.setup.item.driver"
    }
  }

  static func from(setup: SettingsWindowGuidanceSetup) -> SetupItem? {
    switch setup {
    case .services:
      .services
    case .accessibility:
      .accessibility
    case .inputMonitoring:
      .inputMonitoring
    case .driverExtension:
      .driverExtension
    default:
      nil
    }
  }
}

struct SetupPreview {
  let item: SetupItem
  var showingAdvanced = false
  var legacyDriver = false
}

struct SetupView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  @State private var selectedItem: SetupItem = .services

  let preview: SetupPreview?

  init(preview: SetupPreview? = nil) {
    self.preview = preview
    _selectedItem = State(initialValue: preview?.item ?? .services)
  }

  private func itemCompleted(_ item: SetupItem) -> Bool {
    preview == nil && contentViewStates.setupItemCompleted(item)
  }

  var body: some View {
    HStack(spacing: 0) {
      List(SetupItem.allCases, selection: $selectedItem) { item in
        Label(
          title: {
            AppLocalizedText(item.title)
              .lineLimit(nil)
              .fixedSize(horizontal: false, vertical: true)
          },
          icon: {
            Image(systemName: setupStatusSystemImage(item))
              .frame(width: 20.0)
          }
        )
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 8)
        .listRowSeparator(.visible, edges: .bottom)
        .tag(item)
      }
      .frame(width: 250)
      .listStyle(.sidebar)

      Divider()

      ScrollView {
        Group {
          if itemCompleted(selectedItem) {
            setupCompletedMessageView(selectedItem)
          } else {
            switch selectedItem {
            case .services:
              SetupServicesView(
                guidanceContextOverride: preview == nil
                  ? nil
                  : SettingsWindowGuidanceContext(
                    coreDaemonsEnabled: false, coreAgentsEnabled: false))
            case .accessibility:
              if setupItemWaitingForAnotherSetup(.accessibility) {
                setupServicesFirstView()
              } else {
                SetupAccessibilityView()
              }
            case .inputMonitoring:
              if setupItemWaitingForAnotherSetup(.inputMonitoring) {
                setupAccessibilityFirstView()
              } else {
                SetupInputMonitoringView()
              }
            case .driverExtension:
              if setupItemWaitingForAnotherSetup(.driverExtension) {
                setupServicesFirstView()
              } else {
                if preview?.legacyDriver == true {
                  SetupDriverExtensionViewMacOS14(
                    showingAdvanced: preview?.showingAdvanced ?? false)
                } else if #available(macOS 15.0, *) {
                  SetupDriverExtensionView(showingAdvanced: preview?.showingAdvanced ?? false)
                } else {
                  SetupDriverExtensionViewMacOS14(showingAdvanced: preview?.showingAdvanced ?? false)
                }
              }
            }
          }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .padding()
      }
    }
    .onAppear {
      guard preview == nil else { return }
      selectedItem = contentViewStates.setupSelection
    }
    .onChange(of: selectedItem) { newValue in
      guard preview == nil else { return }
      contentViewStates.userSelectedSetupItem(newValue)
    }
    .onChange(of: contentViewStates.setupSelection) { newValue in
      guard preview == nil else { return }
      if selectedItem != newValue {
        selectedItem = newValue
      }
    }
  }

  private func setupStatusSystemImage(_ item: SetupItem) -> String {
    if itemCompleted(item) {
      return "checkmark.circle.fill"
    }

    if setupItemWaitingForAnotherSetup(item) {
      return "hourglass.circle"
    }

    return "circle"
  }

  @ViewBuilder
  private func setupCompletedMessageView(_ item: SetupItem) -> some View {
    VStack(alignment: .leading, spacing: 16.0) {
      AppLocalizedLabel(
        setupCompletedTitle(item),
        systemImage: "checkmark.circle.fill"
      )
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
  }

  private func setupCompletedTitle(_ item: SetupItem) -> String {
    switch item {
    case .services:
      return "settings.setup.completed.services"
    case .accessibility:
      return "settings.setup.completed.accessibility"
    case .inputMonitoring:
      return "settings.setup.completed.input_monitoring"
    case .driverExtension:
      return "settings.setup.completed.driver"
    }
  }

  private func setupItemWaitingForAnotherSetup(_ item: SetupItem) -> Bool {
    if preview != nil { return false }
    switch item {
    case .services:
      return false
    case .accessibility:
      return !itemCompleted(.services)
    case .inputMonitoring:
      return !itemCompleted(.accessibility)
    case .driverExtension:
      return !itemCompleted(.services)
    }
  }

  @ViewBuilder
  private func setupServicesFirstView() -> some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        "settings.setup.services_required",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      Button {
        selectedItem = .services
      } label: {
        AppLocalizedLabel(
          "settings.setup.open_services",
          systemImage: "arrow.left.circle.fill"
        )
      }
      .buttonStyle(.link)
    }
  }

  @ViewBuilder
  private func setupAccessibilityFirstView() -> some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        "settings.setup.accessibility_required",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      Button {
        selectedItem = .accessibility
      } label: {
        AppLocalizedLabel(
          "settings.setup.open_accessibility",
          systemImage: "arrow.left.circle.fill"
        )
      }
      .buttonStyle(.link)
    }
  }
}
