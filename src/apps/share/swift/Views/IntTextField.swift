import SwiftUI

struct IntTextField: View {
  @Binding var value: Int
  // When a formatter is applied directly to a TextField, unintended changes to the content may occur during input.
  // Specifically, the moment the input content is cleared, the minimum value is automatically entered.
  // To avoid this, do not apply the formatter directly to the TextField; instead, apply the formatting in the onChange event.
  @State private var text = ""
  @State private var error = false

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
        Label(
          "must be between \(range.lowerBound) and \(range.upperBound)",
          systemImage: "exclamationmark.circle.fill"
        )
        .modifier(ErrorBorder(padding: 4.0))
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
    if let newText = formatter.string(for: newValue) {
      error = false

      Task { @MainActor in
        if value != newValue {
          value = newValue
        }
        if text != newText {
          text = newText
        }
      }
    } else {
      error = true
    }
  }

  private func update(byText newText: String) {
    if let number = formatter.number(from: newText) {
      error = false

      let newValue = number.intValue
      Task { @MainActor in
        if value != newValue {
          value = newValue
        }
        if text != newText {
          text = newText
        }
      }
    } else {
      error = true
    }
  }
}
