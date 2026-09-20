import SwiftUI

struct CaptureRawInputEventsView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @State private var captureSession: CaptureCoordinator.Session?
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    // The device list is fixed-width; avoid nested split-view size negotiations.
    HStack(spacing: 0) {
      deviceSelector
        .frame(width: 300)

      Divider()

      VStack(alignment: .leading, spacing: 0) {
        VStack(alignment: .leading, spacing: 12) {
          HStack(spacing: 12) {
            if captureCoordinator.capturing {
              Button(role: .destructive) {
                CaptureCoordinator.shared.stopCapture()
              } label: {
                AppLocalizedConstrainedLabel(
                  "event_viewer.capture.stop_escape", systemImage: "stop.fill")
              }
              .keyboardShortcut(.escape, modifiers: [])

              if selectedDeviceIsOpen {
                CaptureActiveLabel(
                  text: localized("event_viewer.capture.raw_events_active")
                )
              } else {
                CaptureWaitingForDeviceAccessLabel()
              }
            } else {
              Button {
                CaptureCoordinator.shared.startCapture()
                focusTestInput()
              } label: {
                AppLocalizedConstrainedLabel(
                  "event_viewer.capture.start", systemImage: "record.circle")
              }
              .disabled(captureCoordinator.rawInputEventsSelectedDeviceId == nil)

              if captureCoordinator.rawInputEventsSelectedDeviceId == nil {
                AppLocalizedText("event_viewer.capture.raw_events_select")
                  .foregroundStyle(.secondary)
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
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    .onReceive(
      NotificationCenter.default.publisher(for: NSApplication.didResignActiveNotification)
    ) { _ in
      if captureCoordinator.capturing {
        CaptureCoordinator.shared.stopCapture()
      }
    }
    .onAppear {
      guard captureSession == nil else {
        return
      }
      captureSession = CaptureCoordinator.shared.begin(.rawInputEvents)
    }
    .onDisappear {
      if let captureSession {
        CaptureCoordinator.shared.end(captureSession)
        self.captureSession = nil
      }
    }
  }

  private var selectedDeviceIsOpen: Bool {
    guard let deviceId = captureCoordinator.rawInputEventsSelectedDeviceId else {
      return false
    }
    return client.hidDeviceIsOpen(deviceId)
  }

  private var emptyMessage: String {
    RawInputCaptureEmptyMessage.make(
      deviceSelected: captureCoordinator.rawInputEventsSelectedDeviceId != nil,
      capturing: captureCoordinator.capturing,
      deviceIsOpen: selectedDeviceIsOpen,
      subject: localized("event_viewer.capture.raw_events"),
      capturingEmptyMessage: localized("event_viewer.capture.raw_events_hint"),
      localized: localized
    )
  }

  private var deviceSelector: some View {
    ConnectedDeviceSelector(
      selection: Binding(
        get: { captureCoordinator.rawInputEventsSelectedDeviceId },
        set: { deviceId in
          captureCoordinator.selectRawInputEventsDevice(deviceId)
        }
      )
    )
  }

  private func focusTestInput() {
    Task { @MainActor in
      // Give SwiftUI a chance to install the text field before requesting focus.
      await Task.yield()
      testInputFocused = true
    }
  }
}
