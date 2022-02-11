import SwiftUI

extension Image {
  func buttonImageStyle() -> some View {
    self
      .padding(.horizontal, 8)
      .padding(.vertical, 2)
      .contentShape(Rectangle())
  }
}
