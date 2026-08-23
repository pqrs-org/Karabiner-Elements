import SwiftUI

struct CaptureRawInputRecordsView: View {
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @ObservedObject private var history = InputReportHistory.shared
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
          if history.capturing {
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
    .onDisappear {
      history.stopCapture()
    }
    .onReceive(NotificationCenter.default.publisher(for: NSApplication.didResignActiveNotification))
    {
      _ in
      if history.capturing {
        history.stopCapture()
      }
    }
  }

  private var captureControls: some View {
    VStack(alignment: .leading, spacing: 12) {
      HStack(spacing: 12) {
        Button(role: .destructive) {
          history.stopCapture()
        } label: {
          Label("Stop capture (Esc)", systemImage: "stop.fill")
        }
        .keyboardShortcut(.escape, modifiers: [])

        if selectedDeviceIsOpen {
          Label("Capturing raw input records", systemImage: "checkmark.circle.fill")
            .foregroundStyle(.green)
        } else {
          ProgressView("Waiting for device access…")
            .controlSize(.small)
            .foregroundStyle(.secondary)
        }
      }

      testInputField
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  private var startCaptureControls: some View {
    VStack(alignment: .leading, spacing: 12) {
      Button {
        history.startCapture()
        focusTestInput()
      } label: {
        Label("Start capture", systemImage: "record.circle")
      }
      .disabled(history.selectedDeviceId == nil)

      if history.selectedDeviceId == nil {
        Text("Select a device to start capturing raw input records.")
          .foregroundStyle(.secondary)
      }

      testInputField

      if !history.entries.isEmpty {
        reportActions
      }
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  private var deviceSelector: some View {
    List(
      selection: Binding(
        get: { history.selectedDeviceId },
        set: { deviceId in
          history.selectDevice(deviceId)
        }
      )
    ) {
      ForEach(client.connectedDevices) { device in
        Label {
          Text(deviceLabelTitle(device))
            .lineLimit(nil)
            .fixedSize(horizontal: false, vertical: true)
        } icon: {
          VStack {
            if device.isKeyboard { Image(systemName: "keyboard") }
            if device.isPointingDevice { Image(systemName: "computermouse") }
            if device.isGamePad { Image(systemName: "gamecontroller") }
            if device.isConsumer { Image(systemName: "headphones") }
          }
          .frame(width: 20)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 8)
        .listRowSeparator(.visible, edges: .bottom)
        .tag(device.id)
      }
    }
    .listStyle(.sidebar)
    .overlay {
      if client.connectedDevices.isEmpty {
        Text("No devices connected.")
          .foregroundStyle(.secondary)
      }
    }
  }

  private var captureActions: some View {
    VStack(alignment: .leading, spacing: 12) {
      reportActions
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
  }

  private var selectedDeviceIsOpen: Bool {
    guard let deviceId = history.selectedDeviceId else {
      return false
    }
    return client.hidDeviceIsOpen(deviceId)
  }

  private var testInputField: some View {
    TextField("Type here to test input", text: $testInput)
      .textFieldStyle(.roundedBorder)
      .focused($testInputFocused)
      .disableAutocorrection(true)
  }

  private func deviceLabelTitle(_ device: EVCoreServiceDaemonClient.ConnectedDevice) -> String {
    var title = device.name
    if !device.manufacturer.isEmpty {
      title += " (\(device.manufacturer))"
    }

    if device.vendorId != 0 || device.productId != 0 {
      title += "\n  [VID: \(device.vendorId), PID: \(device.productId)]"
    }

    return title
  }

  private func focusTestInput() {
    Task { @MainActor in
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
        if history.selectedDeviceId == nil {
          Text("Select the device whose raw input records you want to inspect.")
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else if !history.capturing && history.entries.isEmpty {
          Text("Press Start capture to begin capturing raw input records from the selected device.")
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else if history.capturing && !selectedDeviceIsOpen {
          Text("Waiting for Karabiner-Elements to release exclusive access to the device.")
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else if history.entries.isEmpty {
          Text("No raw input records received.")
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
}
