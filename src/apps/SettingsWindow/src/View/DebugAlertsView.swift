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
    case setupAccessibilityMacOS13
    case setupAccessibilityMacOS27
    case setupAccessibilityCompletedMacOS13
    case setupAccessibilityCompletedMacOS27
    case setupInputMonitoring
    case setupInputMonitoringCompletedMacOS13
    case setupInputMonitoringCompletedMacOS27
    case setupDriverMacOS13
    case setupDriverMacOS13Advanced
    case setupDriverMacOS15
    case setupDriverMacOS15Advanced
    case setupDriverMacOS26
    case setupDriverMacOS26Advanced
    case setupServicesRequired
    case setupAccessibilityRequiredMacOS13
    case setupAccessibilityRequiredMacOS27
    case setupAgentsOnly
    case setupDaemonsOnly
    case setupServicesMacOS13
    case setupServicesMacOS26
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
      case .setupServices, .setupAccessibilityMacOS13, .setupAccessibilityMacOS27,
        .setupAccessibilityCompletedMacOS13, .setupAccessibilityCompletedMacOS27,
        .setupInputMonitoring, .setupInputMonitoringCompletedMacOS13,
        .setupInputMonitoringCompletedMacOS27,
        .setupDriverMacOS13, .setupDriverMacOS13Advanced,
        .setupDriverMacOS15, .setupDriverMacOS15Advanced,
        .setupDriverMacOS26, .setupDriverMacOS26Advanced,
        .setupServicesRequired, .setupAccessibilityRequiredMacOS13,
        .setupAccessibilityRequiredMacOS27, .setupAgentsOnly, .setupDaemonsOnly,
        .setupServicesMacOS13, .setupServicesMacOS26:
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
      case .setupAccessibilityMacOS13:
        return SetupPreview(
          debugItem: .accessibility,
          debugAccessibilityPresentation: .macOS13)
      case .setupAccessibilityMacOS27:
        return SetupPreview(
          debugItem: .accessibility,
          debugAccessibilityPresentation: .macOS27)
      case .setupAccessibilityCompletedMacOS13:
        return SetupPreview(
          debugItem: .accessibility,
          debugCompleted: true,
          debugAccessibilityPresentation: .macOS13)
      case .setupAccessibilityCompletedMacOS27:
        return SetupPreview(
          debugItem: .accessibility,
          debugCompleted: true,
          debugAccessibilityPresentation: .macOS27)
      case .setupInputMonitoring: return SetupPreview(debugItem: .inputMonitoring)
      case .setupInputMonitoringCompletedMacOS13:
        return SetupPreview(
          debugItem: .inputMonitoring,
          debugCompleted: true,
          debugAccessibilityPresentation: .macOS13)
      case .setupInputMonitoringCompletedMacOS27:
        return SetupPreview(
          debugItem: .inputMonitoring,
          debugCompleted: true,
          debugAccessibilityPresentation: .macOS27)
      case .setupDriverMacOS13:
        return SetupPreview(
          debugItem: .driverExtension,
          debugDriverPresentation: .macOS13)
      case .setupDriverMacOS13Advanced:
        return SetupPreview(
          debugItem: .driverExtension,
          debugShowingAdvanced: true,
          debugDriverPresentation: .macOS13)
      case .setupDriverMacOS15:
        return SetupPreview(
          debugItem: .driverExtension,
          debugDriverPresentation: .macOS15)
      case .setupDriverMacOS15Advanced:
        return SetupPreview(
          debugItem: .driverExtension,
          debugShowingAdvanced: true,
          debugDriverPresentation: .macOS15)
      case .setupDriverMacOS26:
        return SetupPreview(
          debugItem: .driverExtension,
          debugDriverPresentation: .macOS26)
      case .setupDriverMacOS26Advanced:
        return SetupPreview(
          debugItem: .driverExtension,
          debugShowingAdvanced: true,
          debugDriverPresentation: .macOS26)
      case .setupServicesRequired:
        return SetupPreview(debugItem: .accessibility, debugWaitingForPrerequisite: true)
      case .setupAccessibilityRequiredMacOS13:
        return SetupPreview(
          debugItem: .inputMonitoring,
          debugWaitingForPrerequisite: true,
          debugAccessibilityPresentation: .macOS13)
      case .setupAccessibilityRequiredMacOS27:
        return SetupPreview(
          debugItem: .inputMonitoring,
          debugWaitingForPrerequisite: true,
          debugAccessibilityPresentation: .macOS27)
      case .setupAgentsOnly:
        return SetupPreview(debugItem: .services, debugAgentsEnabled: true)
      case .setupDaemonsOnly:
        return SetupPreview(debugItem: .services, debugDaemonsEnabled: true)
      case .setupServicesMacOS13:
        return SetupPreview(
          debugItem: .services,
          debugServicesPresentation: .macOS13)
      case .setupServicesMacOS26:
        return SetupPreview(
          debugItem: .services,
          debugServicesPresentation: .macOS26)
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
      case .setupAccessibilityMacOS13:
        return SetupAccessibilityPresentation.macOS13.permissionLocalizationKey
      case .setupAccessibilityMacOS27:
        return SetupAccessibilityPresentation.macOS27.permissionLocalizationKey
      case .setupAccessibilityCompletedMacOS13:
        return SetupAccessibilityPresentation.macOS13.completedLocalizationKey
      case .setupAccessibilityCompletedMacOS27:
        return SetupAccessibilityPresentation.macOS27.completedLocalizationKey
      case .setupInputMonitoring: return "settings.setup.input_monitoring.permission"
      case .setupInputMonitoringCompletedMacOS13:
        return SetupAccessibilityPresentation.macOS13.completedInputMonitoringLocalizationKey
      case .setupInputMonitoringCompletedMacOS27:
        return SetupAccessibilityPresentation.macOS27.completedInputMonitoringLocalizationKey
      case .setupDriverMacOS13: return "settings.setup.driver_legacy.permission"
      case .setupDriverMacOS13Advanced: return "settings.debug.setup_driver_advanced"
      case .setupDriverMacOS15: return "settings.setup.driver.permission"
      case .setupDriverMacOS15Advanced: return "settings.debug.setup_driver_advanced"
      case .setupDriverMacOS26: return "settings.setup.driver.permission"
      case .setupDriverMacOS26Advanced: return "settings.debug.setup_driver_advanced"
      case .setupServicesRequired: return "settings.setup.services_required"
      case .setupAccessibilityRequiredMacOS13:
        return SetupAccessibilityPresentation.macOS13.requiredLocalizationKey
      case .setupAccessibilityRequiredMacOS27:
        return SetupAccessibilityPresentation.macOS27.requiredLocalizationKey
      case .setupAgentsOnly: return "settings.debug.setup_agents_only"
      case .setupDaemonsOnly: return "settings.debug.setup_daemons_only"
      case .setupServicesMacOS13, .setupServicesMacOS26:
        return "settings.setup.services.permission"
      case .profileChangedToast: return "settings.complex_modifications.editor.profile_changed"
      case .rulesChangedToast: return "settings.complex_modifications.editor.rules_changed"
      }
    }

    var macOSVersionRange: String? {
      switch self {
      case .setupAccessibilityMacOS13, .setupAccessibilityCompletedMacOS13,
        .setupInputMonitoringCompletedMacOS13, .setupAccessibilityRequiredMacOS13:
        "macOS 13–26"
      case .setupDriverMacOS13, .setupDriverMacOS13Advanced:
        "macOS 13–14"
      case .setupServicesMacOS13:
        "macOS 13–25"
      case .setupDriverMacOS15, .setupDriverMacOS15Advanced:
        "macOS 15–25"
      case .setupDriverMacOS26, .setupDriverMacOS26Advanced:
        "macOS 26+"
      case .setupServicesMacOS26:
        "macOS 26+"
      case .setupAccessibilityMacOS27, .setupAccessibilityCompletedMacOS27,
        .setupInputMonitoringCompletedMacOS27, .setupAccessibilityRequiredMacOS27:
        "macOS 27+"
      default:
        nil
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
        HStack(spacing: 0) {
          AppLocalizedConstrainedText(alert.title)
          if let macOSVersionRange = alert.macOSVersionRange {
            Text(verbatim: " (\(macOSVersionRange))")
          }
        }
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
    case .setupServices, .setupAccessibilityMacOS13, .setupAccessibilityMacOS27,
      .setupAccessibilityCompletedMacOS13, .setupAccessibilityCompletedMacOS27,
      .setupInputMonitoring, .setupInputMonitoringCompletedMacOS13,
      .setupInputMonitoringCompletedMacOS27,
      .setupDriverMacOS13, .setupDriverMacOS13Advanced,
      .setupDriverMacOS15, .setupDriverMacOS15Advanced,
      .setupDriverMacOS26, .setupDriverMacOS26Advanced,
      .setupServicesRequired, .setupAccessibilityRequiredMacOS13,
      .setupAccessibilityRequiredMacOS27, .setupAgentsOnly, .setupDaemonsOnly,
      .setupServicesMacOS13, .setupServicesMacOS26,
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
