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
    case configuration, services, driver, setup, notifications

    var id: Self { self }

    var title: String {
      switch self {
      case .configuration: return "shared.debug.category.configuration"
      case .services: return "shared.debug.category.services"
      case .driver: return "shared.debug.category.driver"
      case .setup: return "shared.debug.category.setup"
      case .notifications: return "shared.debug.category.notifications"
      }
    }
  }

  enum PreviewAlert: String, CaseIterable, Identifiable {
    case parseError, permissionError, keyboardType
    case servicesStopped, agentsStopped, daemonsStopped
    case agentWaiting, agentRetry, virtualHidWaiting, driverWaiting
    case driverVersion, driverVersionAdvanced
    case setupServices, setupAccessibility, setupInputMonitoring
    case setupDriver, setupDriverAdvanced, setupDriverLegacy, setupDriverLegacyAdvanced
    case setupServicesRequired, setupAccessibilityRequired
    case setupAgentsOnly, setupDaemonsOnly, setupServicesMacOS15
    case setupDriverMacOS15, setupDriverMacOS15Advanced
    case profileChangedToast, rulesChangedToast

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
      case .setupServices: return SetupPreview(item: .services)
      case .setupAccessibility: return SetupPreview(item: .accessibility)
      case .setupInputMonitoring: return SetupPreview(item: .inputMonitoring)
      case .setupDriver: return SetupPreview(item: .driverExtension)
      case .setupDriverAdvanced:
        return SetupPreview(item: .driverExtension, showingAdvanced: true)
      case .setupDriverLegacy:
        return SetupPreview(item: .driverExtension, legacyDriver: true)
      case .setupDriverLegacyAdvanced:
        return SetupPreview(item: .driverExtension, showingAdvanced: true, legacyDriver: true)
      case .setupServicesRequired:
        return SetupPreview(item: .accessibility, waitingForPrerequisite: true)
      case .setupAccessibilityRequired:
        return SetupPreview(item: .inputMonitoring, waitingForPrerequisite: true)
      case .setupAgentsOnly:
        return SetupPreview(item: .services, agentsEnabled: true)
      case .setupDaemonsOnly:
        return SetupPreview(item: .services, daemonsEnabled: true)
      case .setupServicesMacOS15:
        return SetupPreview(item: .services, macOS15Images: true)
      case .setupDriverMacOS15:
        return SetupPreview(item: .driverExtension, macOS15Images: true)
      case .setupDriverMacOS15Advanced:
        return SetupPreview(item: .driverExtension, showingAdvanced: true, macOS15Images: true)
      default: return nil
      }
    }

    var title: String {
      switch self {
      case .parseError: return "settings.configuration.parse_error"
      case .permissionError: return "settings.configuration.permission_error"
      case .keyboardType: return "settings.keyboard_type.select_prompt"
      case .servicesStopped: return "shared.debug.services_stopped"
      case .agentsStopped: return "shared.debug.agents_stopped"
      case .daemonsStopped: return "shared.debug.daemons_stopped"
      case .agentWaiting: return "settings.connection.agent_waiting"
      case .agentRetry: return "shared.debug.agent_retry"
      case .virtualHidWaiting: return "settings.connection.virtual_hid_waiting"
      case .driverWaiting: return "settings.connection.iokit_waiting"
      case .driverVersion: return "settings.driver.restart_required"
      case .driverVersionAdvanced: return "shared.debug.driver_advanced"
      case .setupServices: return "settings.setup.services.permission"
      case .setupAccessibility: return "settings.setup.accessibility.permission"
      case .setupInputMonitoring: return "settings.setup.input_monitoring.permission"
      case .setupDriver: return "settings.setup.driver.permission"
      case .setupDriverAdvanced: return "shared.debug.setup_driver_advanced"
      case .setupDriverLegacy: return "settings.setup.driver_legacy.permission"
      case .setupDriverLegacyAdvanced: return "shared.debug.setup_driver_legacy_advanced"
      case .setupServicesRequired: return "settings.setup.services_required"
      case .setupAccessibilityRequired: return "settings.setup.accessibility_required"
      case .setupAgentsOnly: return "shared.debug.setup_agents_only"
      case .setupDaemonsOnly: return "shared.debug.setup_daemons_only"
      case .setupServicesMacOS15: return "shared.debug.setup_services_macos15"
      case .setupDriverMacOS15: return "shared.debug.setup_driver_macos15"
      case .setupDriverMacOS15Advanced: return "shared.debug.setup_driver_macos15_advanced"
      case .profileChangedToast: return "settings.editor.profile_changed"
      case .rulesChangedToast: return "settings.editor.rules_changed"
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
        SetupView(preview: setup)
          .modifier(LocalizationPreviewInteraction { preview.alert = nil })
      }
    }
  }

  private var previewButtons: some View {
    VStack(alignment: .leading, spacing: 12) {
      AppLocalizedText("shared.debug.alerts")
        .font(.title)
      AppLocalizedText("shared.debug.instructions")
        .foregroundStyle(.secondary)
      AppLocalizedText("shared.debug.preview_hint")
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
            AppLocalizedText(category.title)
          }
          .tag(category)
        }
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
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
        AppLocalizedText("shared.debug.viewed")
        AppLocalizedText(alert.title)
      }
      .toggleStyle(.checkbox)
      .labelsHidden()
      .help(localized("shared.debug.viewed"))

      Button {
        viewedAlerts.insert(alert)
        preview.alert = alert
      } label: {
        AppLocalizedText(alert.title)
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
        parseErrorMessageOverride:
          "karabiner.json: parse error at line 12, column 3: unexpected '}'; expected a value.")
    case .permissionError:
      KarabinerJsonPermissionErrorView()
    case .keyboardType:
      SettingsAlertView()
    case .servicesStopped, .agentsStopped, .daemonsStopped:
      ServicesNotRunningAlertView(
        guidanceContextOverride: SettingsWindowGuidanceContext(
          coreDaemonsRunning: alert == .agentsStopped,
          coreAgentsRunning: alert == .daemonsStopped))
    case .agentWaiting:
      ConsoleUserServerNotConnectedAlertView(disconnectedForAWhileOverride: false)
    case .agentRetry:
      ConsoleUserServerNotConnectedAlertView(disconnectedForAWhileOverride: true)
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
