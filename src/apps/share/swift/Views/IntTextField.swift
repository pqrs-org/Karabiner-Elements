import SwiftUI

struct IntTextField: View {
  @Binding var value: Int
  // When a formatter is applied directly to a TextField, unintended changes to the content may occur during input.
  // Specifically, the moment the input content is cleared, the minimum value is automatically entered.
  // To avoid this, do not apply the formatter directly to the TextField; instead, apply the formatting in the onChange event.
  @State private var text = ""
  @State private var error = false
  @State private var ignoreNextUpdateByText: String?
  @State private var ignoreNextUpdateByValue: Int?

  private let step: Int
  private let range: ClosedRange<Int>
  private let width: CGFloat
  private let formatter: NumberFormatter

  init(
    value: Binding<Int>,
    range: ClosedRange<Int>,
    step: Int,
    width: CGFloat
  ) {
    _value = value
    text = String(value.wrappedValue)

    self.step = step
    self.range = range
    self.width = width

    formatter = NumberFormatter()
    formatter.numberStyle = .none
    formatter.minimum = NSNumber(value: range.lowerBound)
    formatter.maximum = NSNumber(value: range.upperBound)
  }

  var body: some View {
    HStack(spacing: 0) {
      TextField("", text: $text).frame(width: width)

      Stepper(
        value: $value,
        in: range,
        step: step
      ) {
        Text("")
      }.whenHovered { hover in
        if hover {
          // In macOS 13.0.1, if the corresponding TextField has the focus, changing the value by Stepper will not be reflected in the TextField.
          // Therefore, we should remove the focus before Stepper will be clicked.
          Task { @MainActor in
            NSApp.keyWindow?.makeFirstResponder(nil)
          }
        }
      }

      if error {
        Text("must be between \(range.lowerBound) and \(range.upperBound)")
          .foregroundColor(Color.errorForeground)
          .background(Color.errorBackground)
      }
    }
    .onChange(of: text) { newText in
      update(byText: newText)
    }
    .onChange(of: value) { newValue in
      update(byValue: newValue)
    }
  }

  private func update(byValue newValue: Int) {
    let ignore = ignoreNextUpdateByValue == newValue
    ignoreNextUpdateByValue = nil
    if ignore {
      return
    }

    var newText = text
    var newError = true

    if let t = formatter.string(for: newValue) {
      newError = false
      newText = t
    }

    updateProperties(newValue: newValue, newText: newText, newError: newError)
  }

  private func update(byText newText: String) {
    let ignore = ignoreNextUpdateByText == newText
    ignoreNextUpdateByText = nil
    if ignore {
      return
    }

    var newValue = value
    var newError = true

    if let number = formatter.number(from: newText) {
      newError = false
      newValue = number.intValue
    }

    updateProperties(newValue: newValue, newText: newText, newError: newError)
  }

  private func updateProperties(newValue: Int, newText: String, newError: Bool) {
    if value != newValue {
      ignoreNextUpdateByValue = newValue
      value = newValue
    }
    if text != newText {
      ignoreNextUpdateByText = newText
      text = newText
    }

    if error != newError {
      error = newError
    }
  }
}
