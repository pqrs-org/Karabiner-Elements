import SwiftUI

struct VariablesView: View {
  @ObservedObject var evCoreServiceClient = EVCoreServiceClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        HStack(alignment: .center, spacing: 12.0) {
          Button(
            action: {
              let pboard = NSPasteboard.general
              pboard.clearContents()
              pboard.writeObjects([
                evCoreServiceClient.manipulatorEnvironmentStream.text as NSString
              ])
            },
            label: {
              Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
            })

          Button(
            action: {
              libkrbn_core_service_client_async_clear_user_variables()
            },
            label: {
              Label("Clear user variables", systemImage: "clear")
            })
        }
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      RealtimeText(
        stream: evCoreServiceClient.manipulatorEnvironmentStream,
        font: NSFont.monospacedSystemFont(
          ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
          weight: .regular)
      )
      .frame(maxWidth: .infinity, alignment: .leading)
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
    }
    .onAppear {
      evCoreServiceClient.manipulatorEnvironmentStart()
    }
    .onDisappear {
      evCoreServiceClient.manipulatorEnvironmentStop()
    }
  }
}
