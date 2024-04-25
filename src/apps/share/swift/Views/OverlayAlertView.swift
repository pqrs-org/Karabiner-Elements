import SwiftUI

// In principle, native SwiftUI mechanisms such as sheet should be used to display dialogs.
// However, in the case of dialogs that need to be displayed regardless of user interaction,
// such as Input Monitoring permission requests, sheet and the like have the following problems:
//
// - Cannot quit the application while the sheet is displayed
// - Cannot minimize the application while the sheet is displayed
// - Cannot restart OS while the sheet is displayed
//
// This view is intended for use only when such constraints are problematic.

struct OverlayAlertView<Content: View>: View {
  let content: () -> Content

  var body: some View {
    ZStack {
      Rectangle()
        .fill(Color(NSColor.controlBackgroundColor).opacity(0.8))
    }.overlay(
      content()
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .overlay(
          RoundedRectangle(cornerRadius: 16)
            .stroke(Color(NSColor.controlTextColor), lineWidth: 2)
        )
    )
  }
}
