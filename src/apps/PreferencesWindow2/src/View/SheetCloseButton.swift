import SwiftUI

struct SheetCloseButton: View {
  let onCloseButtonPressed: () -> Void

  var body: some View {
    Button(
      action: onCloseButtonPressed
    ) {
      Image(systemName: "xmark.circle")
        .resizable()
        .frame(width: 24.0, height: 24.0)
    }
    .buttonStyle(PlainButtonStyle())
    .offset(x: 10, y: 10)
  }
}
