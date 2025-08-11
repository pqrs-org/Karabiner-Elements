import SwiftUI

struct VariablesView: View {
  @ObservedObject var variablesJsonString = VariablesJsonString.shared

  var body: some View {
    VStack {
      VStack(alignment: .leading, spacing: 12.0) {
        Text("Internal variables of Karabiner-Elements")
          .font(.title)

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
          .font(.custom("Menlo", size: 11.0))
          .padding(5)
          .textSelection(.enabled)
      }
      .frame(maxWidth: .infinity, alignment: .leading)
      .background(Color(NSColor.textBackgroundColor))
    }
  }
}
