import SwiftUI

struct DoubleTextField: View {
  @Binding var value: Double
  // Specifying a formatter directly in a TextField makes it difficult to enter numbers less than 1.0.
  // Specifically, if a user wants to enter "0.2" and enters a value of "0.", the value becomes "0.0".
  // To avoid this, do not specify the formatter directly in the TextField, but use onChange to format the value.
  @State private var text = ""
  @State private var error = false
  @State private var ignoreNextUpdateByText: String?
  @State private var ignoreNextUpdateByValue: Double?

  private let step: Double
  private let range: ClosedRange<Double>
  private let width: CGFloat
  private let maximumFractionDigits: Int
  private let formatter: NumberFormatter

  init(
    value: Binding<Double>,
    range: ClosedRange<Double>,
    step: Double,
    maximumFractionDigits: Int,
    width: CGFloat
  ) {
    _value = value
    text = String(value.wrappedValue)

    self.step = step
    self.range = range
    self.width = width
    self.maximumFractionDigits = maximumFractionDigits

    formatter = NumberFormatter()
    formatter.numberStyle = .decimal  // Use .decimal number style for double values
    formatter.minimum = NSNumber(value: range.lowerBound)
    formatter.maximum = NSNumber(value: range.upperBound)
    formatter.maximumFractionDigits = maximumFractionDigits
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
        Text(
          String(
            format: "must be between %.\(maximumFractionDigits)f and %.\(maximumFractionDigits)f",
            range.lowerBound,
            range.upperBound)
        )
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

  private func update(byValue newValue: Double) {
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
      newValue = number.doubleValue
    }

    updateProperties(newValue: newValue, newText: newText, newError: newError)
  }

  private func updateProperties(newValue: Double, newText: String, newError: Bool) {
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
