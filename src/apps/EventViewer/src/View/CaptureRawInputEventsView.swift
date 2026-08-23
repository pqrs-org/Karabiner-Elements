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
                Label(
                  "Capturing raw input events without Karabiner-Elements modifications.",
                  systemImage: "checkmark.circle.fill"
                )
                .foregroundStyle(.green)
              } else {
                ProgressView("Waiting for device access…")
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

          testInputField
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
      eventHistory.start()
      eventHistory.pause(false)
      eventHistory.clear()
      defer {
        eventHistory.stopRawInputEventsCapture()
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

  private var selectedDeviceIsOpen: Bool {
    guard let deviceId = eventHistory.rawInputEventsSelectedDeviceId else {
      return false
    }
    return client.hidDeviceIsOpen(deviceId)
  }

  private var emptyMessage: String {
    if eventHistory.rawInputEventsSelectedDeviceId == nil {
      return "Select the device whose raw input events you want to inspect."
    }
    if eventHistory.rawInputEventsCapturing {
      return "Type in the test input field to inspect raw input events."
    }
    return "Press Start capture to begin capturing raw input events."
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
