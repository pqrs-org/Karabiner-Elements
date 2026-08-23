import SwiftUI

struct CaptureTestInputField: View {
  @Binding var text: String
  let focus: FocusState<Bool>.Binding

  var body: some View {
    TextField("Type here to test input", text: $text)
      .textFieldStyle(.roundedBorder)
      .focused(focus)
      .disableAutocorrection(true)
  }
}
