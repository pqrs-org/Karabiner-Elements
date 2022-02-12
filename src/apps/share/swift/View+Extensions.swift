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
}
