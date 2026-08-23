import SwiftUI

struct CaptureInputEventsView: View {
  @ObservedObject var eventHistory = EventHistory.shared
  @ObservedObject var evCoreServiceDaemonClient = EVCoreServiceDaemonClient.shared

  var body: some View {
    HSplitView {
      deviceSelector
        .frame(minWidth: 300, maxWidth: 300)

      VStack(alignment: .leading, spacing: 0.0) {
        VStack(alignment: .leading, spacing: 12.0) {
          if eventHistory.mainCapturing {
            HStack(spacing: 12) {
              Button(role: .destructive) {
                eventHistory.stopMainCapture()
              } label: {
                Label("Stop capture (Esc)", systemImage: "stop.circle")
              }
              .keyboardShortcut(.escape, modifiers: [])

              if eventHistory.mainSelectedDeviceId == nil {
                Label(
                  "Capturing input values from all accessible devices.",
                  systemImage: "checkmark.circle.fill"
                )
                .foregroundStyle(.green)
              } else if let deviceId = eventHistory.mainSelectedDeviceId,
                evCoreServiceDaemonClient.hidDeviceIsOpen(deviceId)
              {
                Label(
                  "Capturing raw input values without Karabiner-Elements modifications.",
                  systemImage: "checkmark.circle.fill"
                )
                .foregroundStyle(.green)
              } else {
                ProgressView("Waiting for device access…")
                  .controlSize(.small)
                  .foregroundStyle(.secondary)
              }
            }
          } else {
            Button {
              eventHistory.startMainCapture()
            } label: {
              Label("Start capture", systemImage: "record.circle")
            }
          }

          HStack(alignment: .center, spacing: 12.0) {
            Menu {
              Button("JSON") {
                eventHistory.copyToPasteboardJSON()
              }

              Button("TSV") {
                eventHistory.copyToPasteboardTSV()
              }
            } label: {
              Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
            }
            .disabled(eventHistory.entries.isEmpty)

            Button(
              action: {
                eventHistory.clear()
              },
              label: {
                Label("Clear", systemImage: "clear")
              }
            )
            .disabled(eventHistory.entries.isEmpty)

            Spacer()
          }
        }
        .padding()

        ScrollViewReader { proxy in
          ScrollView {
            if eventHistory.entries.count == 0 {
              Text(emptyMessage)
                .padding(12.0)
                .frame(maxWidth: .infinity, alignment: .leading)

            } else {
              VStack(alignment: .leading, spacing: 0.0) {
                ForEach($eventHistory.entries) { $entry in
                  HStack(alignment: .center, spacing: 12.0) {
                    VStack(alignment: .leading, spacing: 2.0) {
                      Text(entry.eventType)
                        .font(.title2)
                        .frame(width: 70, alignment: .leading)

                      Text(entry.timestampString)
                        .font(.caption)
                        .monospacedDigit()
                        .foregroundStyle(.secondary)
                    }

                    VStack(alignment: .leading, spacing: 2.0) {
                      Text(entry.name)

                      if !entry.misc.isEmpty {
                        Text(entry.misc)
                          .font(.caption)
                      }

                      Text("from \(entry.product)")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)

                    Divider()

                    VStack(alignment: .trailing, spacing: 0) {
                      HStack(alignment: .bottom, spacing: 0) {
                        Text("integer value: ")
                          .font(.caption)
                        Text(entry.integerValue)
                          .font(.callout)
                          .monospaced()
                      }

                      Text("")
                        .font(.callout)
                        .monospaced()
                    }
                    .frame(alignment: .leading)

                    Divider()

                    VStack(alignment: .trailing, spacing: 0) {
                      if entry.usagePage.count > 0 {
                        HStack(alignment: .bottom, spacing: 0) {
                          Text("usage page: ")
                            .font(.caption)
                          Text(entry.usagePage)
                            .font(.callout)
                            .monospaced()
                        }
                      }
                      if entry.usage.count > 0 {
                        HStack(alignment: .bottom, spacing: 0) {
                          Text("usage: ")
                            .font(.caption)
                          Text(entry.usage)
                            .font(.callout)
                            .monospaced()
                        }
                      }
                    }
                    .frame(alignment: .leading)
                  }
                  .padding(.horizontal, 12.0)

                  Divider().id("divider \(entry.id)")
                }
              }
            }
          }
          .background(Color(NSColor.textBackgroundColor))
          .border(Color(NSColor.separatorColor), width: 2)
          .onAppear {
            if let last = eventHistory.entries.last {
              proxy.scrollTo("divider \(last.id)", anchor: .bottom)
            }
          }
          .onChange(of: eventHistory.entries) { newEntries in
            if let last = newEntries.last {
              proxy.scrollTo("divider \(last.id)", anchor: .bottom)
            }
          }
        }
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    .onReceive(NotificationCenter.default.publisher(for: NSApplication.didResignActiveNotification))
    { _ in
      if eventHistory.mainCapturing {
        eventHistory.stopMainCapture()
      }
    }
    .task {
      eventHistory.start()
      eventHistory.pause(false)
      defer {
        eventHistory.stop()
        eventHistory.mainViewDisappeared()
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

  private var emptyMessage: String {
    if eventHistory.mainCapturing {
      return "Press keys on the device to inspect their input values."
    }
    return "Press Start capture to begin capturing input values."
  }

  private var deviceSelector: some View {
    List(
      selection: Binding(
        get: { eventHistory.mainSelectedDeviceId ?? 0 },
        set: { eventHistory.selectMainDevice($0 == 0 ? nil : $0) }
      )
    ) {
      Label("All", systemImage: "circle.grid.2x2")
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 8)
        .listRowSeparator(.visible, edges: .bottom)
        .tag(UInt64(0))

      ForEach(evCoreServiceDaemonClient.connectedDevices) { device in
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
}
