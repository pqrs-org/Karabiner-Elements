import SwiftUI

struct CaptureRawInputRecordsView: View {
  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @ObservedObject private var history = InputReportHistory.shared
  @State private var captureSession: CaptureCoordinator.Session?
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 12) {
        Text("Capture Raw Input Records")
          .font(.title)

        Label {
          VStack(alignment: .leading, spacing: 8) {
            Text(
              "Input reports let you observe signals sent directly by HID hardware. They may reveal events that are omitted from the input values normally used by Karabiner-Elements."
            )

            Text(
              "If pressing a key produces a detectable change here, Karabiner-Elements may be able to support that key."
            )
          }
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
          if captureCoordinator.capturing {
            captureControls
            captureActions
          } else {
            startCaptureControls
          }

          reports
        }
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
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
    .onReceive(
      NotificationCenter.default.publisher(for: NSApplication.didResignActiveNotification)
    ) { _ in
      if captureCoordinator.capturing {
        CaptureCoordinator.shared.stopCapture()
      }
    }
  }

  private var captureControls: some View {
    VStack(alignment: .leading, spacing: 12) {
      HStack(spacing: 12) {
        Button(role: .destructive) {
          CaptureCoordinator.shared.stopCapture()
        } label: {
          Label("Stop capture (Esc)", systemImage: "stop.fill")
        }
        .keyboardShortcut(.escape, modifiers: [])

        if selectedDeviceIsOpen {
          CaptureActiveLabel(
            text: "Capturing raw input records without Karabiner-Elements modifications."
          )
        } else {
          CaptureWaitingForDeviceAccessLabel()
        }
      }

      CaptureTestInputField(text: $testInput, focus: $testInputFocused)
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  private var startCaptureControls: some View {
    VStack(alignment: .leading, spacing: 12) {
      Button {
        CaptureCoordinator.shared.startCapture()
        focusTestInput()
      } label: {
        Label("Start capture", systemImage: "record.circle")
      }
      .disabled(captureCoordinator.rawInputRecordsSelectedDeviceId == nil)

      if captureCoordinator.rawInputRecordsSelectedDeviceId == nil {
        Text("Select a device to start capturing raw input records.")
          .foregroundStyle(.secondary)
      }

      CaptureTestInputField(text: $testInput, focus: $testInputFocused)

      if !history.entries.isEmpty {
        reportActions
      }
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
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

  private var captureActions: some View {
    VStack(alignment: .leading, spacing: 12) {
      reportActions
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  private var selectedDeviceIsOpen: Bool {
    guard let deviceId = captureCoordinator.rawInputRecordsSelectedDeviceId else {
      return false
    }
    return client.hidDeviceIsOpen(deviceId)
  }

  private func focusTestInput() {
    Task { @MainActor in
      // Give SwiftUI a chance to install the text field before requesting focus.
      await Task.yield()
      testInputFocused = true
    }
  }

  private var reportActions: some View {
    HStack(spacing: 12) {
      Menu {
        Button("JSON") {
          history.copyToPasteboardJSON()
        }

        Button("TSV") {
          history.copyToPasteboardTSV()
        }
      } label: {
        Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
      }
      .disabled(history.entries.isEmpty)

      Button {
        history.clear()
      } label: {
        Label("Clear", systemImage: "clear")
      }
      .disabled(history.entries.isEmpty)
    }
  }

  private var reports: some View {
    ScrollViewReader { proxy in
      ScrollView {
        if history.entries.isEmpty {
          Text(emptyMessage)
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          LazyVStack(alignment: .leading, spacing: 0, pinnedViews: [.sectionHeaders]) {
            Section {
              ForEach(history.entries) { entry in
                HStack(alignment: .firstTextBaseline, spacing: 12) {
                  Text(entry.timestampString)
                    .font(.caption)
                    .monospacedDigit()
                    .foregroundStyle(.secondary)
                    .frame(width: 90, alignment: .leading)

                  Divider()

                  Text(entry.reportIdString)
                    .font(.caption)
                    .monospaced()
                    .frame(width: 70, alignment: .leading)

                  Divider()

                  Text(entry.bytesString)
                    .monospaced()
                    .textSelection(.enabled)
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 4)

                Divider().id(entry.id)
              }
            } header: {
              VStack(alignment: .leading, spacing: 0) {
                HStack(alignment: .center, spacing: 12) {
                  Text("Timestamp")
                    .frame(width: 90, alignment: .leading)

                  Divider()

                  Text("Report ID")
                    .frame(width: 70, alignment: .leading)

                  Divider()

                  Text("Data")
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .font(.caption)
                .foregroundStyle(.secondary)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)

                Divider()
              }
              .background(Color(NSColor.textBackgroundColor))
            }
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
      .onChange(of: history.entries) { entries in
        if let last = entries.last {
          proxy.scrollTo(last.id, anchor: .bottom)
        }
      }
    }
  }

  private var emptyMessage: String {
    RawInputCaptureEmptyMessage.make(
      deviceSelected: captureCoordinator.rawInputRecordsSelectedDeviceId != nil,
      capturing: captureCoordinator.capturing,
      deviceIsOpen: selectedDeviceIsOpen,
      subject: "raw input records",
      capturingEmptyMessage: "No raw input records received."
    )
  }
}
