import SwiftUI

@MainActor
final class DebugAlertPreviewState: ObservableObject {
  static let shared = DebugAlertPreviewState()
  @Published var alert: DebugAlertsView.PreviewAlert?
}

struct DebugAlertsView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var preview = DebugAlertPreviewState.shared
  @State private var viewedAlerts: Set<PreviewAlert> = []
  @State private var selectedCategory: PreviewCategory = .configuration

  fileprivate enum PreviewCategory: String, CaseIterable, Identifiable {
    case configuration
    case services
    case driver
    case setup
    case notifications

    var id: Self { self }

    var title: String {
      switch self {
      case .configuration: return "settings.debug.category.configuration"
      case .services: return "settings.debug.category.services"
      case .driver: return "settings.debug.category.driver"
      case .setup: return "settings.debug.category.setup"
      case .notifications: return "settings.debug.category.notifications"
      }
    }
  }

  enum PreviewAlert: String, CaseIterable, Identifiable {
    //
    // .configuration
    //
    case parseError
    case permissionError
    case keyboardType
    //
    // .services
    //
    case servicesStopped
    case agentsStopped
    case daemonsStopped
    case agentWaiting
    case agentRetry
    //
    // .driver
    //
    case virtualHidWaiting
    case driverWaiting
    case driverVersion
    case driverVersionAdvanced
    //
    // .setup
    //
    case setupServices
    case setupAccessibility
    case setupInputMonitoring
    case setupDriver
    case setupDriverAdvanced
    case setupDriverLegacy
    case setupDriverLegacyAdvanced
    case setupServicesRequired
    case setupAccessibilityRequired
    case setupAgentsOnly
    case setupDaemonsOnly
    case setupServicesMacOS15
    case setupDriverMacOS15
    case setupDriverMacOS15Advanced
    //
    // .notifications
    //
    case profileChangedToast
    case rulesChangedToast

    var id: Self { self }

    fileprivate var category: PreviewCategory {
      switch self {
      case .parseError, .permissionError, .keyboardType:
        return .configuration
      case .servicesStopped, .agentsStopped, .daemonsStopped, .agentWaiting, .agentRetry:
        return .services
      case .virtualHidWaiting, .driverWaiting, .driverVersion, .driverVersionAdvanced:
        return .driver
      case .setupServices, .setupAccessibility, .setupInputMonitoring,
        .setupDriver, .setupDriverAdvanced, .setupDriverLegacy, .setupDriverLegacyAdvanced,
        .setupServicesRequired, .setupAccessibilityRequired, .setupAgentsOnly, .setupDaemonsOnly,
        .setupServicesMacOS15, .setupDriverMacOS15, .setupDriverMacOS15Advanced:
        return .setup
      case .profileChangedToast, .rulesChangedToast:
        return .notifications
      }
    }

    var isToast: Bool {
      self == .profileChangedToast || self == .rulesChangedToast
    }

    var setup: SetupPreview? {
      switch self {
      case .setupServices: return SetupPreview(debugItem: .services)
      case .setupAccessibility: return SetupPreview(debugItem: .accessibility)
      case .setupInputMonitoring: return SetupPreview(debugItem: .inputMonitoring)
      case .setupDriver: return SetupPreview(debugItem: .driverExtension)
      case .setupDriverAdvanced:
        return SetupPreview(debugItem: .driverExtension, debugShowingAdvanced: true)
      case .setupDriverLegacy:
        return SetupPreview(debugItem: .driverExtension, debugLegacyDriver: true)
      case .setupDriverLegacyAdvanced:
        return SetupPreview(
          debugItem: .driverExtension, debugShowingAdvanced: true, debugLegacyDriver: true)
      case .setupServicesRequired:
        return SetupPreview(debugItem: .accessibility, debugWaitingForPrerequisite: true)
      case .setupAccessibilityRequired:
        return SetupPreview(debugItem: .inputMonitoring, debugWaitingForPrerequisite: true)
      case .setupAgentsOnly:
        return SetupPreview(debugItem: .services, debugAgentsEnabled: true)
      case .setupDaemonsOnly:
        return SetupPreview(debugItem: .services, debugDaemonsEnabled: true)
      case .setupServicesMacOS15:
        return SetupPreview(debugItem: .services, debugMacOS15Images: true)
      case .setupDriverMacOS15:
        return SetupPreview(debugItem: .driverExtension, debugMacOS15Images: true)
      case .setupDriverMacOS15Advanced:
        return SetupPreview(
          debugItem: .driverExtension, debugShowingAdvanced: true, debugMacOS15Images: true)
      default: return nil
      }
    }

    var title: String {
      switch self {
      case .parseError: return "settings.setup.configuration.parse_error"
      case .permissionError: return "settings.setup.configuration.permission_error"
      case .keyboardType: return "settings.setup.keyboard_type.select_prompt"
      case .servicesStopped: return "settings.debug.services_stopped"
      case .agentsStopped: return "settings.debug.agents_stopped"
      case .daemonsStopped: return "settings.debug.daemons_stopped"
      case .agentWaiting: return "settings.setup.connection.agent_waiting"
      case .agentRetry: return "settings.debug.agent_retry"
      case .virtualHidWaiting: return "settings.setup.connection.virtual_hid_waiting"
      case .driverWaiting: return "settings.setup.connection.iokit_waiting"
      case .driverVersion: return "settings.setup.driver.restart_required"
      case .driverVersionAdvanced: return "settings.debug.driver_advanced"
      case .setupServices: return "settings.setup.services.permission"
      case .setupAccessibility: return "settings.setup.accessibility.permission"
      case .setupInputMonitoring: return "settings.setup.input_monitoring.permission"
      case .setupDriver: return "settings.setup.driver.permission"
      case .setupDriverAdvanced: return "settings.debug.setup_driver_advanced"
      case .setupDriverLegacy: return "settings.setup.driver_legacy.permission"
      case .setupDriverLegacyAdvanced: return "settings.debug.setup_driver_legacy_advanced"
      case .setupServicesRequired: return "settings.setup.services_required"
      case .setupAccessibilityRequired: return "settings.setup.accessibility_required"
      case .setupAgentsOnly: return "settings.debug.setup_agents_only"
      case .setupDaemonsOnly: return "settings.debug.setup_daemons_only"
      case .setupServicesMacOS15: return "settings.debug.setup_services_macos15"
      case .setupDriverMacOS15: return "settings.debug.setup_driver_macos15"
      case .setupDriverMacOS15Advanced: return "settings.debug.setup_driver_macos15_advanced"
      case .profileChangedToast: return "settings.complex_modifications.editor.profile_changed"
      case .rulesChangedToast: return "settings.complex_modifications.editor.rules_changed"
      }
    }
  }

  var body: some View {
    ZStack {
      // Keep the tabs mounted so Setup previews do not discard their scroll positions.
      previewButtons
        .opacity(preview.alert?.setup == nil ? 1 : 0)
        .allowsHitTesting(preview.alert?.setup == nil)
        .accessibilityHidden(preview.alert?.setup != nil)

      if let setup = preview.alert?.setup {
        SetupView(debugPreview: setup)
          .modifier(LocalizationPreviewInteraction { preview.alert = nil })
      }
    }
  }

  private var previewButtons: some View {
    VStack(alignment: .leading, spacing: 12) {
      AppLocalizedText("settings.debug.alerts")
        .font(.title)
      AppLocalizedText("settings.debug.instructions")
        .foregroundStyle(.secondary)
      AppLocalizedText("settings.debug.preview_hint")
        .foregroundStyle(.secondary)

      TabView(selection: $selectedCategory) {
        ForEach(PreviewCategory.allCases) { category in
          ScrollView {
            VStack(alignment: .leading, spacing: 8) {
              ForEach(PreviewAlert.allCases.filter { $0.category == category }) { alert in
                previewRow(alert)
              }
            }
            .padding()
          }
          .tabItem {
            AppLocalizedConstrainedText(category.title)
          }
          .tag(category)
        }
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    // Fixed-size descriptions must not determine the window's minimum height.
    .frame(minHeight: 0, maxHeight: .infinity)
    .buttonStyle(.bordered)
    .padding()
  }

  private func previewRow(_ alert: PreviewAlert) -> some View {
    HStack(spacing: 8) {
      Toggle(
        isOn: Binding(
          get: { viewedAlerts.contains(alert) },
          set: { viewed in
            if viewed {
              viewedAlerts.insert(alert)
            } else {
              viewedAlerts.remove(alert)
            }
          }
        )
      ) {
        AppLocalizedText("settings.debug.viewed")
        AppLocalizedText(alert.title)
      }
      .toggleStyle(.checkbox)
      .labelsHidden()
      .help(localized("settings.debug.viewed"))

      Button {
        viewedAlerts.insert(alert)
        preview.alert = alert
      } label: {
        AppLocalizedConstrainedText(alert.title)
          .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }

  @ViewBuilder
  static func previewView(_ alert: PreviewAlert) -> some View {
    if alert == .permissionError {
      KarabinerJsonPermissionErrorView()
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
        .background(Color(nsColor: .windowBackgroundColor))
    } else {
      OverlayAlertView {
        alertView(alert)
      }
    }
  }

  @ViewBuilder
  private static func alertView(_ alert: PreviewAlert) -> some View {
    switch alert {
    case .parseError:
      DoctorAlertView(
        debugParseErrorMessageOverride:
          "karabiner.json: parse error at line 12, column 3: unexpected '}'; expected a value.")
    case .permissionError:
      KarabinerJsonPermissionErrorView()
    case .keyboardType:
      SettingsAlertView()
    case .servicesStopped, .agentsStopped, .daemonsStopped:
      ServicesNotRunningAlertView(
        debugGuidanceContextOverride: SettingsWindowGuidanceContext(
          coreDaemonsRunning: alert == .agentsStopped,
          coreAgentsRunning: alert == .daemonsStopped))
    case .agentWaiting:
      ConsoleUserServerNotConnectedAlertView(debugDisconnectedForAWhileOverride: false)
    case .agentRetry:
      ConsoleUserServerNotConnectedAlertView(debugDisconnectedForAWhileOverride: true)
    case .virtualHidWaiting:
      VirtualHidDeviceServiceClientNotConnectedAlertView()
    case .driverWaiting:
      DriverNotConnectedAlertView()
    case .driverVersion:
      DriverVersionMismatchedAlertView()
    case .driverVersionAdvanced:
      DriverVersionMismatchedAlertView(showingAdvanced: true)
    case .setupServices, .setupAccessibility, .setupInputMonitoring,
      .setupDriver, .setupDriverAdvanced, .setupDriverLegacy, .setupDriverLegacyAdvanced,
      .setupServicesRequired, .setupAccessibilityRequired, .setupAgentsOnly, .setupDaemonsOnly,
      .setupServicesMacOS15, .setupDriverMacOS15, .setupDriverMacOS15Advanced,
      .profileChangedToast, .rulesChangedToast:
      EmptyView()  // Setup and toast previews use their normal presentation instead.
    }
  }
}

// Keep the toast identity stable so unrelated view updates do not restart its timer.
struct DebugToastPreview: View {
  @State private var toast: SettingsToast
  let onDismiss: () -> Void

  init(message: String, onDismiss: @escaping () -> Void) {
    _toast = State(initialValue: SettingsToast(message: message))
    self.onDismiss = onDismiss
  }

  var body: some View {
    ToastView(toast: toast, onDismiss: onDismiss)
  }
}
