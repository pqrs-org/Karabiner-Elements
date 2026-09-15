import SwiftUI

struct CaptureTestInputField: View {
  @AppLocalizationContext private var localized
  @Binding var text: String
  let focus: FocusState<Bool>.Binding

  var body: some View {
    TextField(localized("event_viewer.capture.test_input"), text: $text)
      .textFieldStyle(.roundedBorder)
      .focused(focus)
      .disableAutocorrection(true)
  }
}
