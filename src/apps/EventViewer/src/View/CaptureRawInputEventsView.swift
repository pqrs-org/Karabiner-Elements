import SwiftUI

struct CaptureRawInputEventsView: View {
  @ObservedObject private var eventHistory = EventHistory.shared
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    HSplitView {
      deviceSelector
        .frame(minWidth: 300, maxWidth: 300)

      VStack(alignment: .leading, spacing: 0) {
        VStack(alignment: .leading, spacing: 12) {
          HStack(spacing: 12) {
            if eventHistory.rawInputEventsCapturing {
              Button(role: .destructive) {
                eventHistory.stopRawInputEventsCapture()
              } label: {
                Label("Stop capture (Esc)", systemImage: "stop.fill")
              }
              .keyboardShortcut(.escape, modifiers: [])

              if selectedDeviceIsOpen {
                CaptureActiveLabel(
                  text: "Capturing raw input events without Karabiner-Elements modifications."
                )
              } else {
                ProgressView("Waiting for device access...")
                  .controlSize(.small)
                  .foregroundStyle(.secondary)
              }
            } else {
              Button {
                eventHistory.startRawInputEventsCapture()
                focusTestInput()
              } label: {
                Label("Start capture", systemImage: "record.circle")
              }
              .disabled(eventHistory.rawInputEventsSelectedDeviceId == nil)

              if eventHistory.rawInputEventsSelectedDeviceId == nil {
                Text("Select a device to start capturing raw input events.")
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
    .onReceive(NotificationCenter.default.publisher(for: NSApplication.didResignActiveNotification))
    { _ in
      if eventHistory.rawInputEventsCapturing {
        eventHistory.stopRawInputEventsCapture()
      }
    }
    .task {
      let session = CaptureSessionManager.shared.begin()
      eventHistory.start()
      eventHistory.pause(false)
      eventHistory.stopInputEventsCapture()
      eventHistory.stopRawInputEventsCapture()
      InputReportHistory.shared.stopCapture()
      eventHistory.clear()
      defer {
        if CaptureSessionManager.shared.end(session) {
          eventHistory.stopRawInputEventsCapture()
        }
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

  private var selectedDeviceIsOpen: Bool {
    guard let deviceId = eventHistory.rawInputEventsSelectedDeviceId else {
      return false
    }
    return client.hidDeviceIsOpen(deviceId)
  }

  private var emptyMessage: String {
    RawInputCaptureEmptyMessage.make(
      deviceSelected: eventHistory.rawInputEventsSelectedDeviceId != nil,
      capturing: eventHistory.rawInputEventsCapturing,
      deviceIsOpen: selectedDeviceIsOpen,
      subject: "raw input events",
      capturingEmptyMessage: "Type in the test input field to inspect raw input events."
    )
  }

  private var deviceSelector: some View {
    ConnectedDeviceSelector(
      selection: Binding(
        get: { eventHistory.rawInputEventsSelectedDeviceId },
        set: { deviceId in
          eventHistory.selectRawInputEventsDevice(deviceId)
        }
      )
    )
  }

  private func focusTestInput() {
    Task { @MainActor in
      await Task.yield()
      testInputFocused = true
    }
  }
}
