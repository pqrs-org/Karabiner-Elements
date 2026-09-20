import SwiftUI

struct SecureEventInputWarningView: View {
  var body: some View {
    VStack(spacing: 20) {
      AppLocalizedLabel(
        "event_viewer.secure_input.title",
        systemImage: WarningBorder.icon
      )
      .font(.system(size: 24))

      AppLocalizedText("event_viewer.secure_input.cannot_capture")
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
    }
    .multilineTextAlignment(.center)
    .frame(width: 700)
    .modifier(WarningBorder(padding: 20))
  }
}
