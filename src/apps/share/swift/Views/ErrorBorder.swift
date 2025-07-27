import SwiftUI

struct ErrorBorder: ViewModifier {
  func body(content: Content) -> some View {
    content
      .padding()
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
