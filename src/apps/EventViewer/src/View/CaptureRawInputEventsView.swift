import SwiftUI

struct CaptureRawInputEventsView: View {
  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @State private var captureSession: CaptureCoordinator.Session?
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    HSplitView {
      deviceSelector
        .frame(minWidth: 300, maxWidth: 300)

      VStack(alignment: .leading, spacing: 0) {
        VStack(alignment: .leading, spacing: 12) {
          HStack(spacing: 12) {
            if captureCoordinator.capturing {
              Button(role: .destructive) {
                CaptureCoordinator.shared.stopCapture()
              } label: {
                Label("Stop capture (Esc)", systemImage: "stop.fill")
              }
              .keyboardShortcut(.escape, modifiers: [])

              if selectedDeviceIsOpen {
                CaptureActiveLabel(
                  text: "Capturing raw input events without Karabiner-Elements modifications."
                )
              } else {
                CaptureWaitingForDeviceAccessLabel()
              }
            } else {
              Button {
                CaptureCoordinator.shared.startCapture()
                focusTestInput()
              } label: {
                Label("Start capture", systemImage: "record.circle")
              }
              .disabled(captureCoordinator.rawInputEventsSelectedDeviceId == nil)

              if captureCoordinator.rawInputEventsSelectedDeviceId == nil {
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
      subject: "raw input events",
      capturingEmptyMessage: "Type in the test input field to inspect raw input events."
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
      await Task.yield()
      testInputFocused = true
    }
  }
}
