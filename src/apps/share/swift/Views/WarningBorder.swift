import SwiftUI

struct WarningBorder: ViewModifier {
  func body(content: Content) -> some View {
    content
      .padding()
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
