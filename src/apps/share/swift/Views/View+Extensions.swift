import SwiftUI

@MainActor
extension View {
  func buttonLabelStyle() -> some View {
    self
      .padding(.horizontal, 8)
      .padding(.vertical, 2)
      .contentShape(Rectangle())
  }

  // Standard List dividers are rendered behind the content, so they might disappear when an overlay sits on top.
  // Use listOverlayDivider to ensure the divider is shown even in those cases.
  func listOverlayDivider() -> some View {
    self
      .listRowSeparator(.hidden)
      .listRowBackground(
        Color.clear.overlay(alignment: .bottom) {
          Divider()
        }
      )
  }

  func whenHovered(_ mouseIsInside: @escaping (Bool) -> Void) -> some View {
    modifier(MouseInsideModifier(mouseIsInside))
  }
}
