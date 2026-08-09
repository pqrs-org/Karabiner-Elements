import AppKit
import SwiftUI

struct LogView: View {
  @ObservedObject private var logMessages = LogMessages.shared

  private func renderLog() -> RenderedLog {
    let string = NSMutableAttributedString()
    var entryLocations: [String: Int] = [:]
    let font = NSFont.monospacedSystemFont(
      ofSize: NSFont.systemFontSize,
      weight: .regular)

    for (index, entry) in logMessages.entries.enumerated() {
      if index > 0 {
        string.append(NSAttributedString(string: "\n"))
      }

      let entryLocation = string.length
      entryLocations[entry.id] = entryLocation
      string.append(
        NSAttributedString(
          string: entry.text,
          attributes: [
            .font: font,
            .foregroundColor: NSColor(entry.foregroundColor),
            .backgroundColor: NSColor(entry.backgroundColor),
          ]))
    }

    return RenderedLog(
      attributedString: NSAttributedString(attributedString: string),
      entryLocations: entryLocations)
  }

  var body: some View {
    let renderedLog = renderLog()

    VStack(alignment: .leading, spacing: 0.0) {
      HStack(alignment: .top) {
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

        Spacer()
      }
      .padding()

      LiveSelectableTextView(
        attributedString: renderedLog.attributedString,
        selectionAnchorLocations: renderedLog.entryLocations,
        followsTail: true,
        textContainerInset: NSSize(width: 14, height: 8),
        lineFragmentPadding: 5
      )
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

private struct RenderedLog {
  let attributedString: NSAttributedString
  let entryLocations: [String: Int]
}
