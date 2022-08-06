import SwiftUI

extension Toggle {
  func switchToggleStyle() -> some View {
    self
      .toggleStyle(.switch)
      .controlSize(.small)
      .font(.body)
  }
}
