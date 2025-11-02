import SwiftUI

struct LogView: View {
  @ObservedObject private var logMessages = LogMessages.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      ScrollView {
        ScrollViewReader { proxy in
          VStack(alignment: .leading, spacing: 0) {
            ForEach(logMessages.entries) { e in
              Text(e.text)
                .id(e.id)
                .font(.callout)
                .monospaced()
                .foregroundColor(e.foregroundColor)
                .background(e.backgroundColor)
                .textSelection(.enabled)
            }
          }
          .padding()
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
          .onChange(of: logMessages.entries.count) { _ in
            if let last = logMessages.entries.last {
              proxy.scrollTo(last.id, anchor: .bottom)
            }
          }
        }
      }
      // Setting a background color on the inner VStack triggers a bug in macOS 15 and earlier,
      // where only the top-left corner renders until the logs finish loading,
      // so the background needs to be applied directly to the ScrollView.
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)

      HStack {
        Text("Current time: \(logMessages.currentTimeString)")

        Button(
          action: {
            logMessages.addDivider()
          },
          label: {
            Label("Add divider", systemImage: "scissors")
          })

        Spacer()

        Button(
          action: {
            var text = ""
            for e in logMessages.entries {
              text += e.text + "\n"
            }

            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([text as NSString])
          },
          label: {
            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
          })
      }
      .padding()
    }
    .onAppear {
      logMessages.watch()
    }
    .onDisappear {
      logMessages.unwatch()
    }
  }
}
