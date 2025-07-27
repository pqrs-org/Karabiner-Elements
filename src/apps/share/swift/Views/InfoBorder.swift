import SwiftUI

struct InfoBorder: ViewModifier {
  static let icon = "lightbulb"

  let padding: CGFloat?

  init(padding: CGFloat? = nil) {
    self.padding = padding
  }

  func body(content: Content) -> some View {
    content
      .padding(.all, padding)
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color.infoForeground, lineWidth: 4)
      )
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color.infoBackground, lineWidth: 2)
      )
      .padding(2)
  }
}
