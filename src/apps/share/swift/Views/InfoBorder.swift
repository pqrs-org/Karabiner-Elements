import SwiftUI

struct InfoBorder: ViewModifier {
  func body(content: Content) -> some View {
    content
      .padding()
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
