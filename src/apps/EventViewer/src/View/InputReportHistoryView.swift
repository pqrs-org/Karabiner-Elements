import SwiftUI

struct InputReportHistoryActions: View {
  @ObservedObject private var history = InputReportHistory.shared

  var body: some View {
    HStack(alignment: .center, spacing: 12) {
      Menu {
        Button("JSON") {
          history.copyToPasteboardJSON()
        }

        Button("TSV") {
          history.copyToPasteboardTSV()
        }
      } label: {
        Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
      }
      .disabled(history.entries.isEmpty)

      Button {
        history.clear()
      } label: {
        Label("Clear", systemImage: "clear")
      }
      .disabled(history.entries.isEmpty)
    }
  }
}

struct InputReportHistoryList: View {
  @ObservedObject private var history = InputReportHistory.shared
  let emptyMessage: String

  var body: some View {
    ScrollViewReader { proxy in
      ScrollView {
        if history.entries.isEmpty {
          Text(emptyMessage)
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          LazyVStack(alignment: .leading, spacing: 0, pinnedViews: [.sectionHeaders]) {
            Section {
              ForEach(history.entries) { entry in
                HStack(alignment: .firstTextBaseline, spacing: 12) {
                  Text(entry.timestampString)
                    .font(.caption)
                    .monospacedDigit()
                    .foregroundStyle(.secondary)
                    .frame(width: 90, alignment: .leading)

                  Divider()

                  Text(entry.reportIdString)
                    .font(.caption)
                    .monospaced()
                    .frame(width: 70, alignment: .leading)

                  Divider()

                  Text(entry.bytesString)
                    .monospaced()
                    .textSelection(.enabled)
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 4)

                Divider().id(entry.id)
              }
            } header: {
              VStack(alignment: .leading, spacing: 0) {
                HStack(alignment: .center, spacing: 12) {
                  Text("Timestamp")
                    .frame(width: 90, alignment: .leading)

                  Divider()

                  Text("Report ID")
                    .frame(width: 70, alignment: .leading)

                  Divider()

                  Text("Data")
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .font(.caption)
                .foregroundStyle(.secondary)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)

                Divider()
              }
              .background(Color(NSColor.textBackgroundColor))
            }
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
      .onChange(of: history.entries) { entries in
        if let last = entries.last {
          proxy.scrollTo(last.id, anchor: .bottom)
        }
      }
    }
  }
}
