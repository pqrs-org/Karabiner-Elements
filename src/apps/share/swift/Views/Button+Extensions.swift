import SwiftUI

@MainActor
extension Button {
  // `role: .destructive` does not change the button color, so change the button color explicitly.
  func deleteButtonStyle() -> some View {
    // Do not put padding here.
    // The padding area ignores click.
    // Use `buttonLabelStyle` in order to set padding.

    modifier(DeleteButtonStyleModifier())
  }
}

private struct DeleteButtonStyleModifier: ViewModifier {
  @Environment(\.isEnabled) private var isEnabled

  func body(content: Content) -> some View {
    if isEnabled {
      content.buttonStyle(DeleteButtonStyle())
    } else {
      content
    }
  }
}

private struct DeleteButtonStyle: ButtonStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .background(Color.red)
      .foregroundColor(.white)
      .cornerRadius(5)
      .shadow(radius: 1)
      .opacity(configuration.isPressed ? 0.8 : 1)
  }
}
