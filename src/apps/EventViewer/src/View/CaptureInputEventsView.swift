import SwiftUI

struct CaptureInputEventsView: View {
  @AppLocalizationContext private var localized
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
              AppLocalizedConstrainedLabel("event_viewer.capture.stop", systemImage: "stop.fill")
            }

            CaptureActiveLabel(text: localized("event_viewer.capture.active"))
          } else {
            Button {
              CaptureCoordinator.shared.startCapture()
              focusTestInput()
            } label: {
              AppLocalizedConstrainedLabel(
                "event_viewer.capture.start", systemImage: "record.circle")
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
      return localized("event_viewer.capture.input_hint")
    }
    return localized("event_viewer.capture.input_start_hint")
  }

  private func focusTestInput() {
    Task { @MainActor in
      // Give SwiftUI a chance to install the text field before requesting focus.
      await Task.yield()
      testInputFocused = true
    }
  }
}
