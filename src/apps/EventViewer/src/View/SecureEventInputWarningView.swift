import SwiftUI

struct SecureEventInputWarningView: View {
  private let messages = [
    "Keyboard events cannot be captured while Secure Keyboard Entry is enabled by another application.",
    "Raw input capture is stopped for safety.",
    "Switch to or quit that application to resume capturing.",
  ]

  var body: some View {
    VStack(spacing: 20) {
      Label(
        "Secure Keyboard Entry is enabled",
        systemImage: WarningBorder.icon
      )
      .font(.system(size: 24))

      Grid(alignment: .leading, horizontalSpacing: 8, verticalSpacing: 8) {
        ForEach(messages, id: \.self) { message in
          GridRow(alignment: .firstTextBaseline) {
            Text("•")
            Text(message)
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
