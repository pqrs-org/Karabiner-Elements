import SwiftUI

struct DevicesView: View {
  @ObservedObject var devicesJsonString = DevicesJsonString.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Button(
          action: {
            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([devicesJsonString.text as NSString])
          },
          label: {
            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
          })
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      ScrollView {
        Text(devicesJsonString.text)
          .lineLimit(nil)
          .textSelection(.enabled)
          .font(.callout)
          .monospaced()
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
          .background(Color(NSColor.textBackgroundColor))
      }
      .border(Color(NSColor.separatorColor), width: 2)
    }
  }
}
