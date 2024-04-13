import SwiftUI

struct MainView: View {
  @ObservedObject var notificationMessage = NotificationMessage.shared
  @State var opacity = 1.0

  var body: some View {
    HStack(alignment: .top) {
      Image(decorative: "app")
        .resizable()
        .frame(width: 48.0, height: 48.0)
        .padding(.leading, 2.0)
      Text(notificationMessage.text)
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

struct MainView_Previews: PreviewProvider {
  @State private static var text = "hello\nworld"

  static var previews: some View {
    Group {
      MainView()
        .previewLayout(.sizeThatFits)
    }
  }
}
