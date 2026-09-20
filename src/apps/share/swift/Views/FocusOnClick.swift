import SwiftUI

struct FocusOnClick: ViewModifier {
  @FocusState private var focused: Bool

  func body(content: Content) -> some View {
    // On macOS 27, clicking a row can change selection while focus stays in the other list,
    // leaving the selection highlight inactive. Explicitly focus the clicked list.
    // A simultaneous tap preserves native selection and also handles clicks on the selected row;
    // observing selection changes would miss those clicks and react to programmatic changes.
    content
      .focused($focused)
      .simultaneousGesture(
        TapGesture().onEnded {
          focused = true
        }
      )
  }
}
