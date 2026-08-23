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

            CaptureActiveLabel(text: "Capturing input events")
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
