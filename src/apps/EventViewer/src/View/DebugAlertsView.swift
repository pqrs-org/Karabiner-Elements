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

  enum PreviewAlert: String, CaseIterable, Identifiable {
    case inputMonitoring
    case secureInput

    var id: Self { self }

    var title: String {
      switch self {
      case .inputMonitoring: return "event_viewer.input_monitoring.title"
      case .secureInput: return "event_viewer.secure_input.title"
      }
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      AppLocalizedText("settings.debug.alerts")
        .font(.title)
      AppLocalizedText("settings.debug.instructions")
        .foregroundStyle(.secondary)
      AppLocalizedText("settings.debug.preview_hint")
        .foregroundStyle(.secondary)
      ForEach(PreviewAlert.allCases) { alert in
        previewRow(alert)
      }
      Spacer()
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
        AppLocalizedText(alert.title)
          .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }

  @ViewBuilder
  static func previewView(_ alert: PreviewAlert) -> some View {
    switch alert {
    case .inputMonitoring:
      OverlayAlertView {
        InputMonitoringAlertView()
      }
    case .secureInput:
      OverlayAlertView(showsBorder: false) {
        SecureEventInputWarningView()
      }
    }
  }
}
