import SwiftUI

// `.tint` and `.role` are available since macOS 12.0, so we cannot use it in order to support macOS 11.0.

extension Button {
  func deleteButtonStyle() -> some View {
    self
      .buttonStyle(PlainButtonStyle())
      .padding(.horizontal, 8)
      .padding(.vertical, 2)
      .background(Color.red)
      .foregroundColor(.white)
      .cornerRadius(5)
      .shadow(radius: 1)
  }
}
