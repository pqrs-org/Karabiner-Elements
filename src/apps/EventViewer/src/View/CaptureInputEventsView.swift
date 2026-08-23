import SwiftUI

struct CaptureInputEventsView: View {
  @ObservedObject private var eventHistory = EventHistory.shared
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 12) {
        HStack(spacing: 12) {
          if eventHistory.inputEventsCapturing {
            Button(role: .destructive) {
              eventHistory.stopInputEventsCapture()
            } label: {
              Label("Stop capture", systemImage: "stop.fill")
            }

            Label("Capturing input events", systemImage: "checkmark.circle.fill")
              .foregroundStyle(.green)
          } else {
            Button {
              eventHistory.startInputEventsCapture()
              focusTestInput()
            } label: {
              Label("Start capture", systemImage: "record.circle")
            }
          }
        }

        testInputField
        InputEventHistoryActions()
      }
      .padding()

      InputEventHistoryList(emptyMessage: emptyMessage)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    .task {
      eventHistory.start()
      eventHistory.pause(false)
      eventHistory.startInputEventsCapture()
      focusTestInput()
      defer {
        eventHistory.stopInputEventsCapture()
        eventHistory.stop()
      }

      do {
        while true {
          try Task.checkCancellation()
          try await Task.sleep(for: .seconds(1))
        }
      } catch {
      }
    }
  }

  private var testInputField: some View {
    TextField("Type here to test input", text: $testInput)
      .textFieldStyle(.roundedBorder)
      .focused($testInputFocused)
      .disableAutocorrection(true)
  }

  private var emptyMessage: String {
    if eventHistory.inputEventsCapturing {
      return "Type in the test input field to inspect input events."
    }
    return "Press Start capture to begin capturing input events."
  }

  private func focusTestInput() {
    Task { @MainActor in
      await Task.yield()
      testInputFocused = true
    }
  }
}

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
