import SwiftUI

struct NotificationView: View {
    @ObservedObject var notificationMessage = NotificationMessage.shared
    let window: NSWindow!

    var body: some View {
        ZStack(alignment: .topLeading) {
            HStack(alignment: .center) {
                Image(decorative: "app")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                    .padding(.leading, 10.0)
                Text(notificationMessage.text)
                    .font(.body)
                    .multilineTextAlignment(.leading)
                    .frame(width: 340.0, height: 64.0, alignment: .leading)
                Spacer()
            }.padding(4.0)

            Button(
                action: { window.orderOut(self) }
            ) {
                Image(decorative: "ic_cancel_18pt")
                    .resizable()
                    .frame(width: 24.0, height: 24.0)
                    .foregroundColor(Color.gray)
            }.buttonStyle(PlainButtonStyle())
        }.background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color(NSColor.windowBackgroundColor))
        )
    }
}

struct NotificationView_Previews: PreviewProvider {
    @State private static var text = "hello\nworld"

    static var previews: some View {
        Group {
            NotificationView(window: nil)
                .previewLayout(.fixed(width: 400.0, height: 64.0))
        }
    }
}
