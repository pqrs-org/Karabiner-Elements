import SwiftUI

struct VirtualKeyboardView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("Keyboard Type")) {
          VStack(alignment: .leading, spacing: 6.0) {
            KeyboardTypeSelectorView()
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("Mouse key")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              Text("Cursor speed:")

              IntTextField(
                value: $settings.virtualHIDKeyboardMouseKeyXYScale,
                range: 0...100_000,
                step: 10,
                width: 50)

              Text("%")
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
