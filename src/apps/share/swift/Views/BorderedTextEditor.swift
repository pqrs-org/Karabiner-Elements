import SwiftUI

struct BorderedTextEditor: View {
  @Binding var text: String
  @FocusState private var focused: Bool

  var body: some View {
    TextEditor(text: $text)
      .focused($focused)
      .background(Color(NSColor.textBackgroundColor))
      .overlay(
        RoundedRectangle(cornerRadius: 4)
          .inset(by: -4)
          .stroke(
            focused ? AnyShapeStyle(.tint) : AnyShapeStyle(.separator),
            lineWidth: 1)
      )
  }
}
