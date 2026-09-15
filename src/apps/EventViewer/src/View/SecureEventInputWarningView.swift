import SwiftUI

struct SecureEventInputWarningView: View {
  private let messages = [
    "event_viewer.secure_input.cannot_capture",
    "event_viewer.secure_input.stopped",
    "event_viewer.secure_input.resume",
  ]

  var body: some View {
    VStack(spacing: 20) {
      AppLocalizedLabel(
        "event_viewer.secure_input.title",
        systemImage: WarningBorder.icon
      )
      .font(.system(size: 24))

      Grid(alignment: .leading, horizontalSpacing: 8, verticalSpacing: 8) {
        ForEach(messages, id: \.self) { message in
          GridRow(alignment: .firstTextBaseline) {
            Text("•")
            AppLocalizedText(message)
          }
        }
      }
      .multilineTextAlignment(.leading)
      .frame(maxWidth: .infinity, alignment: .leading)
    }
    .multilineTextAlignment(.center)
    .frame(width: 700)
    .modifier(WarningBorder(padding: 20))
  }
}
