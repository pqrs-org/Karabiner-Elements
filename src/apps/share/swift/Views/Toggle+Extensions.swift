import SwiftUI

extension Toggle {
  func switchToggleStyle(
    controlSize: ControlSize = .small,
    font: Font = .body
  ) -> some View {
    self
      .toggleStyle(.switch)
      .controlSize(controlSize)
      .font(font)
  }
}
