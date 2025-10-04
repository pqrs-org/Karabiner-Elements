import SwiftUI

struct VariablesView: View {
  @ObservedObject var variablesJsonString = VariablesJsonString.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack {
        Button(
          action: {
            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([variablesJsonString.text as NSString])
          },
          label: {
            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
          })
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      ScrollView {
        Text(variablesJsonString.text)
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
    .onAppear {
      VariablesJsonString.shared.start()
    }
    .onDisappear {
      VariablesJsonString.shared.stop()
    }
  }
}
