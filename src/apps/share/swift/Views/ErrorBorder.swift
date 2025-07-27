import SwiftUI

struct ErrorBorder: ViewModifier {
  static let icon = "exclamationmark.circle.fill"

  let padding: CGFloat?

  init(padding: CGFloat? = nil) {
    self.padding = padding
  }

  func body(content: Content) -> some View {
    content
      .padding(.all, padding)
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color.errorForeground, lineWidth: 4)
      )
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color.errorBackground, lineWidth: 2)
      )
      .padding(2)
  }
}
