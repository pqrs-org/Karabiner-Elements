import SwiftUI

extension View {
  func buttonLabelStyle() -> some View {
    self
      .padding(.horizontal, 8)
      .padding(.vertical, 2)
      .contentShape(Rectangle())
  }

  func sidebarButtonLabelStyle() -> some View {
    self
      .padding(10.0)
      .contentShape(Rectangle())
  }

  func whenHovered(_ mouseIsInside: @escaping (Bool) -> Void) -> some View {
    modifier(MouseInsideModifier(mouseIsInside))
  }
}
