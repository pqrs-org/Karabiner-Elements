import SwiftUI

struct IntTextField: View {
  @Binding var value: Int

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
      TextField("", value: $value, formatter: formatter).frame(width: width)

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
          DispatchQueue.main.async {
            NSApp.keyWindow?.makeFirstResponder(nil)
          }
        }
      }
    }
  }
}
