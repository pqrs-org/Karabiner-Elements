import SwiftUI

struct CaptureRawInputRecordsView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @State private var captureSession: CaptureCoordinator.Session?
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 12) {
        AppLocalizedText("event_viewer.sidebar.raw_input_records")
          .font(.title)

        Label {
          AppLocalizedText("event_viewer.capture.reports_description")
        } icon: {
          Image(systemName: InfoBorder.icon)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .modifier(InfoBorder())
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

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
                  AppLocalizedConstrainedLabel(
                    "event_viewer.capture.stop_escape", systemImage: "stop.fill")
                }
                .keyboardShortcut(.escape, modifiers: [])

                if selectedDeviceIsOpen {
                  CaptureActiveLabel(
                    text: localized("event_viewer.capture.raw_records_active")
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
                .disabled(captureCoordinator.rawInputRecordsSelectedDeviceId == nil)

                if captureCoordinator.rawInputRecordsSelectedDeviceId == nil {
                  AppLocalizedText("event_viewer.capture.raw_records_select")
                    .foregroundStyle(.secondary)
                }
              }
            }

            CaptureTestInputField(text: $testInput, focus: $testInputFocused)
            InputReportHistoryActions()
          }
          .padding()

          InputReportHistoryList(emptyMessage: emptyMessage)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
      }
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
      captureSession = CaptureCoordinator.shared.begin(.rawInputRecords)
    }
    .onDisappear {
      if let captureSession {
        CaptureCoordinator.shared.end(captureSession)
        self.captureSession = nil
      }
    }
  }

  private var selectedDeviceIsOpen: Bool {
    guard let deviceId = captureCoordinator.rawInputRecordsSelectedDeviceId else {
      return false
    }
    return client.hidDeviceIsOpen(deviceId)
  }

  private var emptyMessage: String {
    RawInputCaptureEmptyMessage.make(
      deviceSelected: captureCoordinator.rawInputRecordsSelectedDeviceId != nil,
      capturing: captureCoordinator.capturing,
      deviceIsOpen: selectedDeviceIsOpen,
      subject: localized("event_viewer.capture.raw_records"),
      capturingEmptyMessage: localized("event_viewer.capture.raw_records_empty"),
      localized: localized
    )
  }

  private var deviceSelector: some View {
    ConnectedDeviceSelector(
      selection: Binding(
        get: { captureCoordinator.rawInputRecordsSelectedDeviceId },
        set: { deviceId in
          captureCoordinator.selectRawInputRecordsDevice(deviceId)
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
