import SwiftUI

struct SystemExtensionsView: View {
  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("States")) {
        VStack(alignment: .leading, spacing: 4.0) {
          HStack(alignment: .center, spacing: 12.0) {
            Button(
              action: {
                let pboard = NSPasteboard.general
                pboard.clearContents()
                pboard.writeObjects([SystemExtensions.shared.stream.text as NSString])
              },
              label: {
                Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
              })

            Button(
              action: {
                SystemExtensions.shared.update()
              },
              label: {
                Label("Refresh", systemImage: "arrow.clockwise.circle")
              })
          }

          RealtimeTextWithProgress(
            stream: SystemExtensions.shared.stream,
            font: NSFont.monospacedSystemFont(
              ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
              weight: .regular)
          )
          .frame(height: 160)
          .background(Color(NSColor.textBackgroundColor))
          .border(Color(NSColor.separatorColor), width: 2)
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      GroupBox(label: Text("macOS log messages")) {
        VStack(alignment: .leading, spacing: 4.0) {
          HStack(alignment: .center, spacing: 12.0) {
            Button(
              action: {
                let pboard = NSPasteboard.general
                pboard.clearContents()
                pboard.writeObjects([SysextdLogMessages.shared.stream.text as NSString])
              },
              label: {
                Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
              })

            Button(
              action: {
                SysextdLogMessages.shared.update()
              },
              label: {
                Label("Refresh", systemImage: "arrow.clockwise.circle")
              })
          }

          RealtimeTextWithProgress(
            stream: SysextdLogMessages.shared.stream,
            font: NSFont.monospacedSystemFont(
              ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
              weight: .regular)
          )
          .background(Color(NSColor.textBackgroundColor))
          .border(Color(NSColor.separatorColor), width: 2)
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    .padding()
    .onAppear {
      SystemExtensions.shared.update()
      SysextdLogMessages.shared.update()
    }
  }
}
