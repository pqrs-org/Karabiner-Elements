import SwiftUI

struct SystemExtensionsView: View {
  @ObservedObject private var systemExtensionsStreamer = SystemExtensions.shared.streamer
  @ObservedObject private var sysextdLogStreamer = SysextdLogMessages.shared.streamer

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: AppLocalizedText("settings.system_extensions.states")) {
        VStack(alignment: .leading, spacing: 4.0) {
          HStack(alignment: .center, spacing: 12.0) {
            Button(
              action: {
                let pboard = NSPasteboard.general
                pboard.clearContents()
                pboard.writeObjects([systemExtensionsStreamer.text as NSString])
              },
              label: {
                AppLocalizedConstrainedLabel(
                  "shared.action.copy_to_pasteboard", systemImage: "arrow.right.doc.on.clipboard")
              })

            Button(
              action: {
                SystemExtensions.shared.update()
              },
              label: {
                AppLocalizedConstrainedLabel(
                  "shared.action.refresh", systemImage: "arrow.clockwise.circle")
              })
          }

          LiveSelectableTextView(
            text: systemExtensionsStreamer.text,
            font: NSFont.monospacedSystemFont(
              ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
              weight: .regular),
            isLoading: !systemExtensionsStreamer.isTextReady
          )
          .frame(height: 160)
          .background(Color(NSColor.textBackgroundColor))
          .border(Color(NSColor.separatorColor), width: 2)
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      GroupBox(label: AppLocalizedText("settings.system_extensions.logs")) {
        VStack(alignment: .leading, spacing: 4.0) {
          HStack(alignment: .center, spacing: 12.0) {
            Button(
              action: {
                let pboard = NSPasteboard.general
                pboard.clearContents()
                pboard.writeObjects([sysextdLogStreamer.text as NSString])
              },
              label: {
                AppLocalizedConstrainedLabel(
                  "shared.action.copy_to_pasteboard", systemImage: "arrow.right.doc.on.clipboard")
              })

            Button(
              action: {
                SysextdLogMessages.shared.update()
              },
              label: {
                AppLocalizedConstrainedLabel(
                  "shared.action.refresh", systemImage: "arrow.clockwise.circle")
              })
          }

          LiveSelectableTextView(
            text: sysextdLogStreamer.text,
            font: NSFont.monospacedSystemFont(
              ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
              weight: .regular),
            isLoading: !sysextdLogStreamer.isTextReady
          )
          .background(Color(NSColor.textBackgroundColor))
          .border(Color(NSColor.separatorColor), width: 2)
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    // Fixed-size group titles must not determine the window's minimum height.
    .frame(minHeight: 0, maxHeight: .infinity)
    .padding()
    .onAppear {
      SystemExtensions.shared.update()
      SysextdLogMessages.shared.update()
    }
  }
}
