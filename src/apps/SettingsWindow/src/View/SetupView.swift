import SwiftUI

enum SetupItem: String, CaseIterable, Identifiable, Hashable {
  case services
  case accessibility
  case inputMonitoring
  case driverExtension

  var id: Self { self }

  func title(accessibilityPresentation: SetupAccessibilityPresentation) -> String {
    switch self {
    case .services: return "settings.setup.item.services"
    case .accessibility: return accessibilityPresentation.itemLocalizationKey
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
  let debugItem: SetupItem
  let debugCompleted: Bool
  let debugShowingAdvanced: Bool
  let debugWaitingForPrerequisite: Bool
  let debugAgentsEnabled: Bool
  let debugDaemonsEnabled: Bool
  let debugServicesPresentation: SetupServicesPresentation?
  let debugDriverPresentation: SetupDriverPresentation?
  let debugAccessibilityPresentation: SetupAccessibilityPresentation?

  init(
    debugItem: SetupItem,
    debugCompleted: Bool = false,
    debugShowingAdvanced: Bool = false,
    debugWaitingForPrerequisite: Bool = false,
    debugAgentsEnabled: Bool = false,
    debugDaemonsEnabled: Bool = false,
    debugServicesPresentation: SetupServicesPresentation? = nil,
    debugDriverPresentation: SetupDriverPresentation? = nil,
    debugAccessibilityPresentation: SetupAccessibilityPresentation? = nil
  ) {
    self.debugItem = debugItem
    self.debugCompleted = debugCompleted
    self.debugShowingAdvanced = debugShowingAdvanced
    self.debugWaitingForPrerequisite = debugWaitingForPrerequisite
    self.debugAgentsEnabled = debugAgentsEnabled
    self.debugDaemonsEnabled = debugDaemonsEnabled
    self.debugServicesPresentation = debugServicesPresentation
    self.debugDriverPresentation = debugDriverPresentation
    self.debugAccessibilityPresentation = debugAccessibilityPresentation
  }
}

struct SetupView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  @State private var selectedItem: SetupItem = .services

  let debugPreview: SetupPreview?

  init(debugPreview: SetupPreview? = nil) {
    self.debugPreview = debugPreview
    _selectedItem = State(initialValue: debugPreview?.debugItem ?? .services)
  }

  private func itemCompleted(_ item: SetupItem) -> Bool {
    if let debugPreview {
      return debugPreview.debugCompleted && item == debugPreview.debugItem
    }

    return contentViewStates.setupItemCompleted(item)
  }

  private var accessibilityPresentation: SetupAccessibilityPresentation {
    debugPreview?.debugAccessibilityPresentation ?? .current
  }

  private var servicesPresentation: SetupServicesPresentation {
    debugPreview?.debugServicesPresentation ?? .current
  }

  private var driverPresentation: SetupDriverPresentation {
    debugPreview?.debugDriverPresentation ?? .current
  }

  var body: some View {
    HStack(spacing: 0) {
      List(SetupItem.allCases, selection: $selectedItem) { item in
        Label(
          title: {
            AppLocalizedText(item.title(accessibilityPresentation: accessibilityPresentation))
              .lineLimit(nil)
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
                debugGuidanceContextOverride: debugPreview == nil
                  ? nil
                  : LocalServicesGuidanceContext(
                    coreDaemonsEnabled: debugPreview?.debugDaemonsEnabled,
                    coreAgentsEnabled: debugPreview?.debugAgentsEnabled),
                presentation: servicesPresentation)
            case .accessibility:
              if setupItemWaitingForAnotherSetup(.accessibility) {
                setupServicesFirstView()
              } else {
                SetupAccessibilityView(presentation: accessibilityPresentation)
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
                if driverPresentation == .macOS13 {
                  SetupDriverExtensionViewMacOS13(
                    showingAdvanced: debugPreview?.debugShowingAdvanced ?? false,
                    presentation: driverPresentation)
                } else {
                  SetupDriverExtensionView(
                    showingAdvanced: debugPreview?.debugShowingAdvanced ?? false,
                    presentation: driverPresentation)
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
      guard debugPreview == nil else { return }
      selectedItem = contentViewStates.setupSelection
    }
    .onChange(of: selectedItem) { newValue in
      guard debugPreview == nil else { return }
      contentViewStates.userSelectedSetupItem(newValue)
    }
    .onChange(of: contentViewStates.setupSelection) { newValue in
      guard debugPreview == nil else { return }
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
      return accessibilityPresentation.completedLocalizationKey
    case .inputMonitoring:
      return accessibilityPresentation.completedInputMonitoringLocalizationKey
    case .driverExtension:
      return "settings.setup.completed.driver"
    }
  }

  private func setupItemWaitingForAnotherSetup(_ item: SetupItem) -> Bool {
    if let debugPreview {
      return debugPreview.debugWaitingForPrerequisite && item == debugPreview.debugItem
    }
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
        AppLocalizedConstrainedLabel(
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
        accessibilityPresentation.requiredLocalizationKey,
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      Button {
        selectedItem = .accessibility
      } label: {
        AppLocalizedConstrainedLabel(
          accessibilityPresentation.openSetupLocalizationKey,
          systemImage: "arrow.left.circle.fill"
        )
      }
      .buttonStyle(.link)
    }
  }
}
