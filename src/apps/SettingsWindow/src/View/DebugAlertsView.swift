import SwiftUI

@MainActor
final class DebugAlertPreviewState: ObservableObject {
  static let shared = DebugAlertPreviewState()
  @Published var alert: DebugAlertsView.PreviewAlert?
}

struct DebugAlertsView: View {
  @ObservedObject private var preview = DebugAlertPreviewState.shared

  enum PreviewAlert: String, CaseIterable, Identifiable {
    case parseError, permissionError, keyboardType
    case servicesStopped, agentsStopped, daemonsStopped
    case agentWaiting, agentRetry, virtualHidWaiting, driverWaiting
    case driverVersion, driverVersionAdvanced

    var id: Self { self }

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
      }
    }
  }

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 12) {
        AppLocalizedText("shared.debug.alerts")
          .font(.title)
        AppLocalizedText("shared.debug.instructions")
          .foregroundStyle(.secondary)
        AppLocalizedText("shared.debug.preview_hint")
          .foregroundStyle(.secondary)
        ForEach(PreviewAlert.allCases) { alert in
          Button {
            preview.alert = alert
          } label: {
            AppLocalizedText(alert.title)
              .frame(maxWidth: .infinity, alignment: .leading)
          }
        }
      }
      .buttonStyle(.bordered)
      .padding()
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
    }
  }
}
