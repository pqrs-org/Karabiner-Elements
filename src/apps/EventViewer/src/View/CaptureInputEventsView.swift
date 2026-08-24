import SwiftUI

struct CaptureInputEventsView: View {
  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var eventHistory = EventHistory.shared
  @State private var captureSession: CaptureCoordinator.Session?
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 12) {
        HStack(spacing: 12) {
          if captureCoordinator.capturing {
            Button(role: .destructive) {
              CaptureCoordinator.shared.stopCapture()
            } label: {
              Label("Stop capture", systemImage: "stop.fill")
            }

            CaptureActiveLabel(text: "Capturing input events")
          } else {
            Button {
              CaptureCoordinator.shared.startCapture()
              focusTestInput()
            } label: {
              Label("Start capture", systemImage: "record.circle")
            }
          }
        }

        CaptureTestInputField(text: $testInput, focus: $testInputFocused)
        InputEventHistoryActions()
      }
      .padding()

      InputEventHistoryList(emptyMessage: emptyMessage)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    .onAppear {
      guard captureSession == nil else {
        return
      }
      captureSession = CaptureCoordinator.shared.begin(.inputEvents)
      focusTestInput()
    }
    .onDisappear {
      if let captureSession {
        CaptureCoordinator.shared.end(captureSession)
        self.captureSession = nil
      }
    }
  }

  private var emptyMessage: String {
    if captureCoordinator.capturing {
      return "Type in the test input field to inspect input events."
    }
    return "Press Start capture to begin capturing input events."
  }

  private func focusTestInput() {
    Task { @MainActor in
      // Give SwiftUI a chance to install the text field before requesting focus.
      await Task.yield()
      testInputFocused = true
    }
  }
}
