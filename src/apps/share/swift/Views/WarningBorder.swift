import SwiftUI

struct WarningBorder: ViewModifier {
  static let icon = "exclamationmark.triangle"

  let padding: CGFloat?

  init(padding: CGFloat? = nil) {
    self.padding = padding
  }

  func body(content: Content) -> some View {
    content
      .padding(.all, padding)
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color.warningForeground, lineWidth: 4)
      )
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color.warningBackground, lineWidth: 2)
      )
      .padding(2)
  }
}
