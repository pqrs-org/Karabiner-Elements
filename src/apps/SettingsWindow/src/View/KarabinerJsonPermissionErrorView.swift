import SwiftUI

struct KarabinerJsonPermissionErrorView: View {
  var body: some View {
    VStack(spacing: 20.0) {
      AppLocalizedLabel(
        "settings.configuration.permission_error",
        systemImage: ErrorBorder.icon
      )
      .font(.title)

      AppLocalizedText(
        "settings.configuration.permission_error_hint"
      )
      .multilineTextAlignment(.center)
    }
    .padding()
    .frame(width: 850)
    .textSelection(.enabled)
  }
}
