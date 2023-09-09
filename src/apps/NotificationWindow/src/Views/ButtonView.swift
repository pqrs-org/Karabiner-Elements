import SwiftUI

struct ButtonView: View {
  let mainWindow: NSWindow!
  let buttonWindow: NSWindow!

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
          .foregroundColor(Color.gray)
      }
    )
    .buttonStyle(PlainButtonStyle())
    // Do not set opacity to Button because the mouse click will be ignored.
    .background(
      Circle()
        .fill(Color(NSColor.windowBackgroundColor))
    )
  }
}

struct ButtonView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      ButtonView(mainWindow: nil, buttonWindow: nil)
        .previewLayout(.sizeThatFits)
    }
  }
}
