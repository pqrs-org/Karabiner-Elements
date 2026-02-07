import SwiftUI

struct DevicesView: View {
  @ObservedObject var evCoreServiceClient = EVCoreServiceClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Button(
          action: {
            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([evCoreServiceClient.connectedDevicesStream.text as NSString])
          },
          label: {
            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
          })
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      RealtimeText(
        stream: evCoreServiceClient.connectedDevicesStream,
        font: NSFont.monospacedSystemFont(
          ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
          weight: .regular)
      )
      .frame(maxWidth: .infinity, alignment: .leading)
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
    }
    .onAppear {
      evCoreServiceClient.startConnectedDevices()
    }
    .onDisappear {
      evCoreServiceClient.stopConnectedDevices()
    }
  }
}
