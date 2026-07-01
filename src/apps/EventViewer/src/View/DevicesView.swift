import SwiftUI

struct DevicesView: View {
  @ObservedObject var evCoreServiceDaemonClient = EVCoreServiceDaemonClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Button(
          action: {
            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([evCoreServiceDaemonClient.connectedDevicesStream.text as NSString])
          },
          label: {
            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
          })
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      RealtimeText(
        stream: evCoreServiceDaemonClient.connectedDevicesStream,
        font: NSFont.monospacedSystemFont(
          ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
          weight: .regular)
      )
      .frame(maxWidth: .infinity, alignment: .leading)
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
    }
    .onAppear {
      evCoreServiceDaemonClient.startConnectedDevices()
    }
    .onDisappear {
      evCoreServiceDaemonClient.stopConnectedDevices()
    }
  }
}
