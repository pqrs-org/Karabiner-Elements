import SwiftUI

struct VariablesView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Button(
          action: {
            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([VariablesJsonString.shared.stream.text as NSString])
          },
          label: {
            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
          })
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      RealtimeText(
        stream: VariablesJsonString.shared.stream,
        font: NSFont.monospacedSystemFont(
          ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
          weight: .regular)
      )
      .frame(maxWidth: .infinity, alignment: .leading)
      .border(Color(NSColor.separatorColor), width: 2)
    }
    .onAppear {
      VariablesJsonString.shared.start()
    }
    .onDisappear {
      VariablesJsonString.shared.stop()
    }
  }
}
