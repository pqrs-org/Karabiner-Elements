import AppKit
import SwiftUI

struct LogView: View {
  @ObservedObject private var logMessages = LogMessages.shared
  @State private var filterKeyword = ""
  @State private var filterContextLineCount = 5

  private var trimmedFilterKeyword: String {
    filterKeyword.trimmingCharacters(in: .whitespacesAndNewlines)
  }

  private var filteredEntries: [FilteredLogMessageEntry] {
    let keyword = trimmedFilterKeyword
    if keyword.isEmpty {
      return logMessages.entries.map {
        FilteredLogMessageEntry(logMessageEntry: $0)
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
          matchedKeyword: matchedIndexes.contains(index) ? keyword : nil))

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
      HStack(alignment: .top) {
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

        Spacer()

        VStack(alignment: .trailing) {
          SearchField(text: $filterKeyword)
            .frame(width: 300)

          if !trimmedFilterKeyword.isEmpty {
            Stepper(
              "Show \(filterContextLineCount) surrounding lines",
              value: $filterContextLineCount,
              in: 0...50
            )
          }
        }
      }
      .padding()

      LogTextView(
        entries: filteredEntries,
        followsTail: trimmedFilterKeyword.isEmpty,
        filterKey: "\(trimmedFilterKeyword)\n\(filterContextLineCount)"
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

private struct LogTextView: NSViewRepresentable {
  let entries: [FilteredLogMessageEntry]
  let followsTail: Bool
  let filterKey: String

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  func makeNSView(context: Context) -> NSScrollView {
    let scrollView = NSScrollView()
    scrollView.hasVerticalScroller = true
    scrollView.autohidesScrollers = true
    scrollView.borderType = .noBorder
    scrollView.drawsBackground = true
    scrollView.backgroundColor = .textBackgroundColor

    let textView = NSTextView(frame: .zero)
    textView.isEditable = false
    textView.isSelectable = true
    textView.isRichText = true
    textView.allowsUndo = false
    textView.drawsBackground = true
    textView.backgroundColor = .textBackgroundColor
    textView.textContainerInset = NSSize(width: 14, height: 8)
    textView.minSize = .zero
    textView.maxSize = NSSize(
      width: CGFloat.greatestFiniteMagnitude,
      height: CGFloat.greatestFiniteMagnitude)
    textView.isVerticallyResizable = true
    textView.isHorizontallyResizable = false
    textView.autoresizingMask = [.width]
    textView.textContainer?.containerSize = NSSize(
      width: scrollView.contentSize.width,
      height: CGFloat.greatestFiniteMagnitude)
    textView.textContainer?.widthTracksTextView = true
    scrollView.documentView = textView
    context.coordinator.textView = textView
    context.coordinator.scrollView = scrollView

    return scrollView
  }

  func updateNSView(_ scrollView: NSScrollView, context: Context) {
    context.coordinator.update(
      renderedLog: renderLog(),
      followsTail: followsTail,
      filterKey: filterKey)
  }

  private func renderLog() -> RenderedLog {
    let string = NSMutableAttributedString()
    var entryLocations: [String: Int] = [:]
    let font = NSFont.monospacedSystemFont(
      ofSize: NSFont.systemFontSize,
      weight: .regular)

    for (index, entry) in entries.enumerated() {
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

      for matchedRange in entry.matchedRanges {
        string.addAttributes(
          [
            .foregroundColor: NSColor(Color.infoForeground),
            .backgroundColor: NSColor(Color.infoBackground),
          ],
          range: NSRange(
            location: entryLocation + matchedRange.location,
            length: matchedRange.length))
      }
    }

    return RenderedLog(
      attributedString: NSAttributedString(attributedString: string),
      entryLocations: entryLocations)
  }

  struct RenderedLog {
    let attributedString: NSAttributedString
    let entryLocations: [String: Int]
  }

  @MainActor
  final class Coordinator {
    weak var textView: NSTextView?
    weak var scrollView: NSScrollView?

    private var lastRenderedLog: RenderedLog?
    private var lastFilterKey: String?
    private var followsTail = false
    private var updateGeneration = 0

    func update(renderedLog: RenderedLog, followsTail: Bool, filterKey: String) {
      self.followsTail = followsTail
      let filterChanged = lastFilterKey.map { $0 != filterKey } ?? false
      lastFilterKey = filterKey

      if let lastRenderedLog,
        !filterChanged,
        lastRenderedLog.attributedString.isEqual(to: renderedLog.attributedString)
      {
        return
      }

      let scrollTarget: ScrollTarget? =
        filterChanged ? (followsTail ? .bottom : .top) : nil

      apply(renderedLog, scrollTarget: scrollTarget)
    }

    private func apply(_ renderedLog: RenderedLog, scrollTarget: ScrollTarget?) {
      guard let textView,
        let scrollView
      else {
        return
      }

      let visibleOrigin = scrollView.contentView.bounds.origin
      let wasAtBottom =
        lastRenderedLog == nil
        || scrollView.documentVisibleRect.maxY >= textView.bounds.maxY - 1
      let selectedRanges = adjustedSelectedRanges(
        textView.selectedRanges,
        from: lastRenderedLog,
        to: renderedLog)
      let hasSelection = selectedRanges.contains { $0.rangeValue.length > 0 }

      textView.textStorage?.setAttributedString(renderedLog.attributedString)
      textView.selectedRanges = selectedRanges

      lastRenderedLog = renderedLog
      updateGeneration += 1

      if scrollTarget == .top {
        scheduleLayoutAndScroll(.top, generation: updateGeneration)
      } else if scrollTarget == .bottom || (followsTail && wasAtBottom && !hasSelection) {
        scheduleLayoutAndScroll(.bottom, generation: updateGeneration)
      } else {
        scheduleLayoutAndScroll(.position(visibleOrigin), generation: updateGeneration)
      }
    }

    private func scheduleLayoutAndScroll(
      _ action: ScrollAction,
      generation: Int
    ) {
      DispatchQueue.main.async { [weak self] in
        guard let self,
          generation == updateGeneration,
          let textView,
          let scrollView
        else {
          return
        }

        scrollView.layoutSubtreeIfNeeded()

        if let textContainer = textView.textContainer,
          let layoutManager = textView.layoutManager
        {
          textView.setFrameSize(
            NSSize(
              width: scrollView.contentSize.width,
              height: textView.frame.height))
          layoutManager.ensureLayout(for: textContainer)

          let textHeight = ceil(
            layoutManager.usedRect(for: textContainer).height
              + textView.textContainerInset.height * 2)
          textView.setFrameSize(
            NSSize(
              width: scrollView.contentSize.width,
              height: max(scrollView.contentSize.height, textHeight)))
        }

        switch action {
        case .top:
          scrollView.contentView.scroll(to: .zero)
        case .bottom:
          textView.scrollToEndOfDocument(nil)
        case .position(let origin):
          scrollView.contentView.scroll(to: origin)
        }

        scrollView.reflectScrolledClipView(scrollView.contentView)
        textView.layoutManager?.invalidateDisplay(
          forCharacterRange: NSRange(location: 0, length: textView.string.utf16.count))
        textView.setNeedsDisplay(textView.visibleRect)
        scrollView.contentView.needsDisplay = true
        scrollView.needsDisplay = true
        textView.displayIfNeeded()
        scrollView.displayIfNeeded()
      }
    }

    private func adjustedSelectedRanges(
      _ selectedRanges: [NSValue],
      from oldRenderedLog: RenderedLog?,
      to newRenderedLog: RenderedLog
    ) -> [NSValue] {
      // Log updates may remove old entries from the beginning of the text. Find an entry that
      // exists in both snapshots and use the change in its location to move the selection by the
      // same amount. Finally, clamp both ends because part or all of the selection may have been
      // removed with the old entries.
      let locationDelta =
        oldRenderedLog.flatMap { oldRenderedLog in
          oldRenderedLog.entryLocations.lazy.compactMap { id, oldLocation in
            newRenderedLog.entryLocations[id].map { $0 - oldLocation }
          }.first
        } ?? 0
      let newLength = newRenderedLog.attributedString.length

      return selectedRanges.map { value in
        let range = value.rangeValue
        let location = min(max(0, range.location + locationDelta), newLength)
        let end = min(max(0, range.location + range.length + locationDelta), newLength)

        return NSValue(
          range: NSRange(
            location: location,
            length: max(0, end - location)))
      }
    }

    private enum ScrollTarget {
      case top
      case bottom
    }

    private enum ScrollAction {
      case top
      case bottom
      case position(NSPoint)
    }
  }
}

private struct FilteredLogMessageEntry: Identifiable {
  let id: String
  let text: String
  let matchedRanges: [NSRange]
  let foregroundColor: Color
  let backgroundColor: Color

  init(
    id: String,
    text: String,
    matchedRanges: [NSRange],
    foregroundColor: Color,
    backgroundColor: Color
  ) {
    self.id = id
    self.text = text
    self.matchedRanges = matchedRanges
    self.foregroundColor = foregroundColor
    self.backgroundColor = backgroundColor
  }

  @MainActor
  init(logMessageEntry: LogMessageEntry, matchedKeyword: String? = nil) {
    let messageText = logMessageEntry.text
    id = logMessageEntry.id
    text = messageText
    matchedRanges =
      matchedKeyword.map {
        messageText.localizedCaseInsensitiveRanges(of: $0)
      } ?? []
    foregroundColor = logMessageEntry.foregroundColor
    backgroundColor = logMessageEntry.backgroundColor
  }

  static func omittedLinesMarker(from startIndex: Int, to endIndex: Int) -> FilteredLogMessageEntry
  {
    FilteredLogMessageEntry(
      id: "omittedLines:\(startIndex)-\(endIndex)",
      text: String(repeating: "~", count: 80),
      matchedRanges: [],
      foregroundColor: Color(NSColor.textBackgroundColor),
      backgroundColor: Color(NSColor.textColor))
  }
}

extension String {
  fileprivate func localizedCaseInsensitiveRanges(of keyword: String) -> [NSRange] {
    var result: [NSRange] = []
    var searchRange = startIndex..<endIndex

    while let range = range(
      of: keyword,
      options: [.caseInsensitive],
      range: searchRange,
      locale: .current)
    {
      result.append(NSRange(range, in: self))
      searchRange = range.upperBound..<endIndex
    }

    return result
  }
}
