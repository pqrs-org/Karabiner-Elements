import SwiftUI

struct NotificationMainView: View {
  @ObservedObject private var state = ConsoleUserServerUIState.shared
  @State private var opacity = 1.0

  var body: some View {
    HStack(alignment: .top) {
      Image(nsImage: NSWorkspace.shared.icon(forFile: Bundle.main.bundlePath))
        .resizable()
        .frame(width: 48.0, height: 48.0)
        .padding(.leading, 2.0)
      Text(state.notificationMessage)
        .font(.body)
        .multilineTextAlignment(.leading)
        .fixedSize(horizontal: false, vertical: true)
        .padding(4.0)
        .frame(width: 340.0, alignment: .leading)
    }
    .background(
      RoundedRectangle(cornerRadius: 12)
        .fill(Color(NSColor.windowBackgroundColor))
    )
    .opacity(opacity)
    .whenHovered { hover in
      opacity = hover ? 0.2 : 1.0
    }
  }
}

struct NotificationCloseButtonView: View {
  let mainWindow: NSWindow
  let buttonWindow: NSWindow

  var body: some View {
    Button(
      action: {
        mainWindow.orderOut(self)
        buttonWindow.orderOut(self)
      },
      label: {
        Image(systemName: "xmark.circle")
          .resizable()
          .frame(width: 24.0, height: 24.0)
          .foregroundColor(.gray)
      }
    )
    .buttonStyle(.plain)
    // Do not set opacity to Button because the mouse click will be ignored.
    .background(
      Circle()
        .fill(Color(NSColor.windowBackgroundColor))
    )
  }
}
