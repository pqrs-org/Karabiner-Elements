import SwiftUI

struct InputEventHistoryActions: View {
  @ObservedObject private var eventHistory = EventHistory.shared

  var body: some View {
    HStack(alignment: .center, spacing: 12) {
      Menu {
        Button("JSON") {
          eventHistory.copyToPasteboardJSON()
        }

        Button("TSV") {
          eventHistory.copyToPasteboardTSV()
        }
      } label: {
        Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
      }
      .disabled(eventHistory.entries.isEmpty)

      Button {
        eventHistory.clear()
      } label: {
        Label("Clear", systemImage: "clear")
      }
      .disabled(eventHistory.entries.isEmpty)

      Spacer()
    }
  }
}

struct InputEventHistoryList: View {
  @ObservedObject private var eventHistory = EventHistory.shared
  let emptyMessage: String

  var body: some View {
    ScrollViewReader { proxy in
      ScrollView {
        if eventHistory.entries.isEmpty {
          Text(emptyMessage)
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          VStack(alignment: .leading, spacing: 0) {
            ForEach($eventHistory.entries) { $entry in
              HStack(alignment: .center, spacing: 12) {
                VStack(alignment: .leading, spacing: 2) {
                  Text(entry.eventType)
                    .font(.title2)
                    .frame(width: 70, alignment: .leading)

                  Text(entry.timestampString)
                    .font(.caption)
                    .monospacedDigit()
                    .foregroundStyle(.secondary)
                }

                VStack(alignment: .leading, spacing: 2) {
                  Text(entry.name)

                  if !entry.misc.isEmpty {
                    Text(entry.misc)
                      .font(.caption)
                  }

                  Text("from \(entry.product)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
                }
                .frame(maxWidth: .infinity, alignment: .leading)

                Divider()

                VStack(alignment: .trailing, spacing: 0) {
                  HStack(alignment: .bottom, spacing: 0) {
                    Text("integer value: ")
                      .font(.caption)
                    Text(entry.integerValue)
                      .font(.callout)
                      .monospaced()
                  }

                  Text("")
                    .font(.callout)
                    .monospaced()
                }
                .frame(alignment: .leading)

                Divider()

                VStack(alignment: .trailing, spacing: 0) {
                  if !entry.usagePage.isEmpty {
                    HStack(alignment: .bottom, spacing: 0) {
                      Text("usage page: ")
                        .font(.caption)
                      Text(entry.usagePage)
                        .font(.callout)
                        .monospaced()
                    }
                  }
                  if !entry.usage.isEmpty {
                    HStack(alignment: .bottom, spacing: 0) {
                      Text("usage: ")
                        .font(.caption)
                      Text(entry.usage)
                        .font(.callout)
                        .monospaced()
                    }
                  }
                }
                .frame(alignment: .leading)
              }
              .padding(.horizontal, 12)

              Divider().id("divider \(entry.id)")
            }
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
      .onAppear {
        if let last = eventHistory.entries.last {
          proxy.scrollTo("divider \(last.id)", anchor: .bottom)
        }
      }
      .onChange(of: eventHistory.entries) { entries in
        if let last = entries.last {
          proxy.scrollTo("divider \(last.id)", anchor: .bottom)
        }
      }
    }
  }
}
