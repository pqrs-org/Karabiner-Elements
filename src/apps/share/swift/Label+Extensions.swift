import SwiftUI

extension Label {
  func buttonLabelStyle() -> some View {
    self
      .padding(.horizontal, 8)
      .padding(.vertical, 2)
      .contentShape(Rectangle())
  }
}
