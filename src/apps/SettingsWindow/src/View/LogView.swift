import SwiftUI

struct LogView: View {
  @ObservedObject private var logMessages = LogMessages.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      ScrollView {
        ScrollViewReader { proxy in
          VStack(alignment: .leading, spacing: 0) {
            ForEach(logMessages.entries) { e in
              Text(e.text)
                .id(e.id)
                .font(.custom("Menlo", size: 11.0))
                .foregroundColor(e.foregroundColor)
                .background(e.backgroundColor)
                .textSelection(.enabled)
            }
          }
          .padding()
          .background(Color(NSColor.textBackgroundColor))
          .onChange(of: logMessages.entries.count) { _ in
            if let last = logMessages.entries.last {
              proxy.scrollTo(last.id, anchor: .bottom)
            }
          }
        }
      }

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
