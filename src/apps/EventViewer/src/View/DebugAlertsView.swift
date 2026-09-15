import SwiftUI

@MainActor
final class DebugAlertPreviewState: ObservableObject {
  static let shared = DebugAlertPreviewState()
  @Published var alert: DebugAlertsView.PreviewAlert?
}

struct DebugAlertsView: View {
  @ObservedObject private var preview = DebugAlertPreviewState.shared

  enum PreviewAlert: String, CaseIterable, Identifiable {
    case inputMonitoring, secureInput
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
      Spacer()
    }
    .buttonStyle(.bordered)
    .padding()
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
