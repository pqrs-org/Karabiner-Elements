import SwiftUI

struct DoubleTextField: View {
  @Binding var value: Double
  // Specifying a formatter directly in a TextField makes it difficult to enter numbers less than 1.0.
  // Specifically, if a user wants to enter "0.2" and enters a value of "0.", the value becomes "0.0".
  // To avoid this, do not specify the formatter directly in the TextField, but use onChange to format the value.
  @State private var text = ""

  private let step: Double
  private let range: ClosedRange<Double>
  private let width: CGFloat
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

    formatter = NumberFormatter()
    formatter.numberStyle = .decimal  // Use .decimal number style for double values
    formatter.minimum = NSNumber(value: range.lowerBound)
    formatter.maximum = NSNumber(value: range.upperBound)
    formatter.maximumFractionDigits = maximumFractionDigits
  }

  var body: some View {
    HStack(spacing: 0) {
      TextField("", text: $text).frame(width: width)
        .onChange(of: text) { newText in
          updateValue(newText)
        }
        .onChange(of: value) { newValue in
          updateText(newValue)
        }

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
    }
  }

  private func updateText(_ newValue: Double) {
    if let newText = formatter.string(for: newValue) {
      if text != newText {
        Task { @MainActor in
          text = newText
        }
      }
    }
  }

  private func updateValue(_ newText: String) {
    if let number = formatter.number(from: newText) {
      let newValue = number.doubleValue
      if value != newValue {
        Task { @MainActor in
          value = newValue
        }
      }
    }
  }
}
