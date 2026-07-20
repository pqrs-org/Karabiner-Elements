import SwiftUI

struct LogView: View {
  @ObservedObject private var logMessages = LogMessages.shared
  @State private var filterKeyword = ""
  @State private var filterContextLineCount = 5
  private let bottomAnchorID = "bottomAnchor"

  private var filteredEntries: [FilteredLogMessageEntry] {
    let keyword = filterKeyword.trimmingCharacters(in: .whitespacesAndNewlines)
    if keyword.isEmpty {
      return logMessages.entries.map {
        FilteredLogMessageEntry(logMessageEntry: $0, isMatched: false)
      }
    }

    var indexes = Set<Int>()
    var matchedIndexes = Set<Int>()

    for index in logMessages.entries.indices
    where logMessages.entries[index].text.localizedCaseInsensitiveContains(keyword) {
      matchedIndexes.insert(index)

      let lowerBound = max(logMessages.entries.startIndex, index - filterContextLineCount)
      let upperBound = min(
        logMessages.entries.index(before: logMessages.entries.endIndex),
        index + filterContextLineCount)

      for contextIndex in lowerBound...upperBound {
        indexes.insert(contextIndex)
      }
    }

    var filteredEntries: [FilteredLogMessageEntry] = []
    var previousIndex: Int?

    for index in indexes.sorted() {
      if let previousIndex {
        if index > previousIndex + 1 {
          filteredEntries.append(
            FilteredLogMessageEntry.omittedLinesMarker(
              from: previousIndex + 1,
              to: index - 1))
        }
      } else if index > logMessages.entries.startIndex {
        filteredEntries.append(
          FilteredLogMessageEntry.omittedLinesMarker(
            from: logMessages.entries.startIndex,
            to: index - 1))
      }

      filteredEntries.append(
        FilteredLogMessageEntry(
          logMessageEntry: logMessages.entries[index],
          isMatched: matchedIndexes.contains(index)))

      previousIndex = index
    }

    if let previousIndex,
      previousIndex < logMessages.entries.index(before: logMessages.entries.endIndex)
    {
      filteredEntries.append(
        FilteredLogMessageEntry.omittedLinesMarker(
          from: previousIndex + 1,
          to: logMessages.entries.index(before: logMessages.entries.endIndex)))
    }

    return filteredEntries
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      HStack {
        SearchField(text: $filterKeyword)
          .frame(maxWidth: .infinity)

        Stepper(
          "Show \(filterContextLineCount) surrounding lines",
          value: $filterContextLineCount,
          in: 0...50
        )
        .disabled(filterKeyword.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)

        Button(
          action: {
            var text = ""
            for e in filteredEntries {
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

      ScrollViewReader { proxy in
        ScrollView {
          VStack(alignment: .leading, spacing: 0) {
            ForEach(filteredEntries) { e in
              if e.showsMatchIndicator {
                HStack(spacing: 0) {
                  Rectangle()
                    .fill(Color.infoBackground)
                    .frame(width: 10)

                  LogMessageText(e: e)
                }
                .id(e.id)
              } else {
                LogMessageText(e: e)
                  .id(e.id)
              }
            }

            Color.clear
              .frame(height: 1)
              .id(bottomAnchorID)
          }
          .padding()
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        }
        .onAppear {
          if filterKeyword.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            Task { @MainActor in
              proxy.scrollTo(bottomAnchorID, anchor: .bottom)
            }
          }
        }
        .onChange(of: logMessages.entries) { _ in
          // Keep the filtered view at the current position while new non-matching lines arrive.
          if filterKeyword.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            Task { @MainActor in
              proxy.scrollTo(bottomAnchorID, anchor: .bottom)
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

private struct LogMessageText: View {
  let e: FilteredLogMessageEntry

  var body: some View {
    Text(e.text)
      .font(.callout)
      .monospaced()
      .foregroundColor(e.foregroundColor)
      .background(e.backgroundColor)
      .textSelection(.enabled)
  }
}

private struct FilteredLogMessageEntry: Identifiable {
  let id: String
  let text: String
  let showsMatchIndicator: Bool
  let foregroundColor: Color
  let backgroundColor: Color

  init(
    id: String,
    text: String,
    showsMatchIndicator: Bool,
    foregroundColor: Color,
    backgroundColor: Color
  ) {
    self.id = id
    self.text = text
    self.showsMatchIndicator = showsMatchIndicator
    self.foregroundColor = foregroundColor
    self.backgroundColor = backgroundColor
  }

  @MainActor
  init(logMessageEntry: LogMessageEntry, isMatched: Bool) {
    id = logMessageEntry.id.uuidString
    text = logMessageEntry.text
    // Show an indicator for warn and error logs when they match the filter,
    // instead of changing their colors.
    showsMatchIndicator =
      isMatched
      && (logMessageEntry.logLevel == .warn || logMessageEntry.logLevel == .error)

    if isMatched, !showsMatchIndicator {
      // Use info colors for entries that match the filter.
      foregroundColor = Color.infoForeground
      backgroundColor = Color.infoBackground
    } else {
      foregroundColor = logMessageEntry.foregroundColor
      backgroundColor = logMessageEntry.backgroundColor
    }
  }

  static func omittedLinesMarker(from startIndex: Int, to endIndex: Int) -> FilteredLogMessageEntry
  {
    FilteredLogMessageEntry(
      id: "omittedLines:\(startIndex)-\(endIndex)",
      text: String(repeating: "~", count: 80),
      showsMatchIndicator: false,
      foregroundColor: Color(NSColor.textBackgroundColor),
      backgroundColor: Color(NSColor.textColor))
  }
}
