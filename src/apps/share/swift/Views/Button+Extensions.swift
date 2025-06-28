import SwiftUI

@MainActor
extension Button {
  // `role: .destructive` does not change the button color, so change the button color explicitly.
  func deleteButtonStyle() -> some View {
    // Do not put padding here.
    // The padding area ignores click.
    // Use `buttonLabelStyle` in order to set padding.

    self
      .buttonStyle(PlainButtonStyle())
      .background(Color.red)
      .foregroundColor(.white)
      .cornerRadius(5)
      .shadow(radius: 1)
  }
}
