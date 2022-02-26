import SwiftUI

// `.tint` and `.role` are available since macOS 12.0, so we cannot use it in order to support macOS 11.0.

extension Button {
  func prominentButtonStyle() -> some View {
    // Do not put padding here.
    // The padding area ignores click.
    // Use `buttonLabelStyle` in order to set padding.

    self
      .buttonStyle(PlainButtonStyle())
      .background(Color.blue)
      .foregroundColor(.white)
      .cornerRadius(5)
      .shadow(radius: 1)
  }

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

  func sidebarButtonStyle(selected: Bool) -> some View {
    // Do not put padding here.
    // The padding area ignores click.
    // Use `buttonLabelStyle` in order to set padding.

    self
      .buttonStyle(PlainButtonStyle())
      .if(selected) {
        $0
          .background(Color.blue)
          .foregroundColor(.white)
      }
      .cornerRadius(5)
  }
}
